// Signaling server for WebRTC P2P connections (Deno Deploy)
// Room metadata uses Deno.Kv (create/join only).
// SDP/ICE exchange uses WebSocket relay — zero KV reads during signaling.

const kv = await Deno.openKv();
const ROOM_TTL = 2 * 60 * 60 * 1000; // 2 hours
const MAX_PLAYERS = 8;
const CHARS = "ABCDEFGHJKMNPQRSTUVWXYZ23456789"; // no 0/O/1/I/L

// ── In-memory WebSocket connections per room (per-isolate) ────
// code -> { host: WebSocket|null, clients: Map<playerId, WebSocket> }
const wsRooms = new Map();

// ── Cross-isolate relay via BroadcastChannel ─────────────────
const bc = new BroadcastChannel("signaling");
bc.onmessage = (e) => {
    const { roomCode, ...msg } = e.data;
    relayToLocal(roomCode, msg);
};

function relayToLocal(code, msg) {
    const room = wsRooms.get(code);
    if (!room) return;

    if (msg.type === "offer" && msg.targetPlayerId !== undefined) {
        // Relay offer from host to specific client
        const clientWs = room.clients.get(msg.targetPlayerId);
        if (clientWs && clientWs.readyState === WebSocket.OPEN) {
            clientWs.send(JSON.stringify({ type: "offer", sdp: msg.sdp }));
        }
    } else if (msg.type === "answer") {
        // Relay answer from client to host
        if (room.host && room.host.readyState === WebSocket.OPEN) {
            room.host.send(JSON.stringify({ type: "answer", sdp: msg.sdp, playerId: msg.playerId }));
        }
    } else if (msg.type === "client_joined") {
        // Notify host that a new client joined
        if (room.host && room.host.readyState === WebSocket.OPEN) {
            room.host.send(JSON.stringify({
                type: "client_joined",
                playerId: msg.playerId,
                playerName: msg.playerName,
            }));
        }
    }
}

async function genCode() {
    let code;
    let exists = true;
    while (exists) {
        code = Array.from({ length: 6 }, () =>
            CHARS[Math.floor(Math.random() * CHARS.length)]
        ).join("");
        const res = await kv.get(["rooms", code]);
        exists = !!res.value;
    }
    return code;
}

function json(data, status = 200) {
    return new Response(JSON.stringify(data), {
        status,
        headers: {
            "Content-Type": "application/json",
            "Access-Control-Allow-Origin": "*",
            "Access-Control-Allow-Methods": "GET,POST,DELETE,OPTIONS",
            "Access-Control-Allow-Headers": "Content-Type",
        },
    });
}

Deno.serve(async (req) => {
    if (req.method === "OPTIONS") return json({ ok: true });

    const url = new URL(req.url);
    const parts = url.pathname.split("/").filter(Boolean);

    // GET /health — no KV read
    if (parts[0] === "health" && req.method === "GET") {
        return json({ status: "ok" });
    }

    // ── WebSocket endpoint: GET /ws/:code ────────────────────
    if (parts[0] === "ws" && parts[1]) {
        const code = parts[1].toUpperCase();
        const role = url.searchParams.get("role"); // "host" or "client"
        const playerId = Number(url.searchParams.get("playerId") || "0");

        const { socket, response } = Deno.upgradeWebSocket(req);

        // Get or create the room's in-memory WS state
        if (!wsRooms.has(code)) {
            wsRooms.set(code, { host: null, clients: new Map() });
        }
        const room = wsRooms.get(code);

        if (role === "host") {
            room.host = socket;
        } else {
            room.clients.set(playerId, socket);
        }

        socket.onmessage = (e) => {
            try {
                const msg = JSON.parse(e.data);
                // Relay locally within this isolate
                relayToLocal(code, msg);
                // Relay to other isolates via BroadcastChannel
                bc.postMessage({ roomCode: code, ...msg });
            } catch (err) {
                console.error("WS message parse error:", err);
            }
        };

        socket.onclose = () => {
            const r = wsRooms.get(code);
            if (!r) return;
            if (role === "host") {
                r.host = null;
            } else {
                r.clients.delete(playerId);
            }
            // Clean up empty in-memory room entries
            if (!r.host && r.clients.size === 0) {
                wsRooms.delete(code);
            }
        };

        return response;
    }

    if (parts[0] !== "rooms") return json({ error: "not found" }, 404);

    // ── POST /rooms — create room (1 KV write + genCode reads) ──
    if (parts.length === 1 && req.method === "POST") {
        const { playerName } = await req.json();
        const code = await genCode();
        const roomId = crypto.randomUUID();

        await kv.set(
            ["rooms", code],
            {
                roomId,
                hostName: playerName,
                nextId: 1,
                created: Date.now(),
                players: [{ id: 0, name: playerName }],
            },
            { expireIn: ROOM_TTL }
        );

        return json({ roomCode: code, roomId });
    }

    const code = parts[1]?.toUpperCase();
    const action = parts[2];

    // ── GET /rooms/:code — room info (1 KV read) ────────────
    if (parts.length === 2 && req.method === "GET") {
        const roomRes = await kv.get(["rooms", code]);
        const room = roomRes.value;
        if (!room) return json({ error: "room not found" }, 404);
        return json({
            roomCode: code,
            hostName: room.hostName,
            playerCount: room.players.length,
            roomId: room.roomId,
        });
    }

    // ── DELETE /rooms/:code — close room (1 KV delete) ──────
    if (parts.length === 2 && req.method === "DELETE") {
        await kv.delete(["rooms", code]);
        // Close all WS connections for this room
        const room = wsRooms.get(code);
        if (room) {
            if (room.host && room.host.readyState === WebSocket.OPEN)
                room.host.close();
            for (const ws of room.clients.values()) {
                if (ws.readyState === WebSocket.OPEN) ws.close();
            }
            wsRooms.delete(code);
        }
        return json({ ok: true });
    }

    // ── POST /rooms/:code/join (1-2 KV reads + 1 write) ─────
    if (action === "join" && req.method === "POST") {
        const { playerName } = await req.json();

        let success = false;
        let playerId = -1;
        let retries = 5;
        let hostName = "";

        while (!success && retries > 0) {
            const currentRes = await kv.get(["rooms", code]);
            const currentRoom = currentRes.value;
            if (!currentRoom) return json({ error: "room not found" }, 404);
            if (currentRoom.players.length >= MAX_PLAYERS)
                return json({ error: "room full" }, 400);

            hostName = currentRoom.hostName;
            playerId = currentRoom.nextId++;
            currentRoom.players.push({ id: playerId, name: playerName });

            const commit = await kv
                .atomic()
                .check(currentRes)
                .set(["rooms", code], currentRoom, { expireIn: ROOM_TTL })
                .commit();

            success = commit.ok;
            retries--;
        }

        if (!success) return json({ error: "concurrent join failed" }, 500);

        // Notify host via WebSocket that a new client joined
        const joinMsg = { type: "client_joined", playerId, playerName };
        const room = wsRooms.get(code);
        if (room && room.host && room.host.readyState === WebSocket.OPEN) {
            room.host.send(JSON.stringify(joinMsg));
        }
        // Also broadcast to other isolates in case host is on a different one
        bc.postMessage({ roomCode: code, ...joinMsg });

        return json({ playerId, hostName });
    }

    return json({ error: "not found" }, 404);
});
