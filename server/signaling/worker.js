// Signaling server for WebRTC P2P connections (Deno Deploy)
// Room metadata uses Deno.Kv (create/join only).
// SDP/ICE exchange uses HTTP long polling — zero KV reads during signaling.

const kv = await Deno.openKv();
const ROOM_TTL = 2 * 60 * 60 * 1000; // 2 hours
const MAX_PLAYERS = 8;
const CHARS = "ABCDEFGHJKMNPQRSTUVWXYZ23456789"; // no 0/O/1/I/L

// Per-room in-memory signaling state (zero KV)
// code -> {
//   hostQueue: [],                    // messages waiting for host to poll
//   clientQueues: Map<playerId, []>,  // messages waiting for each client
//   hostResolver: { resolve, timer } | null,
//   clientResolvers: Map<playerId, { resolve, timer }>
// }
const sigRooms = new Map();

function getSigRoom(code) {
    if (!sigRooms.has(code)) {
        sigRooms.set(code, {
            hostQueue: [],
            clientQueues: new Map(),
            hostResolver: null,
            clientResolvers: new Map(),
        });
    }
    return sigRooms.get(code);
}

function cleanupSigRoom(code) {
    const room = sigRooms.get(code);
    if (!room) return;
    if (
        !room.hostResolver &&
        room.hostQueue.length === 0 &&
        room.clientResolvers.size === 0 &&
        Array.from(room.clientQueues.values()).every((q) => q.length === 0)
    ) {
        sigRooms.delete(code);
    }
}

// ── Cross-isolate relay via BroadcastChannel ─────────────────
const bc = new BroadcastChannel("signaling");
bc.onmessage = (e) => {
    const { roomCode, ...msg } = e.data;
    relayToLocal(roomCode, msg);
};

function relayToLocal(code, msg) {
    const room = getSigRoom(code);

    if (msg.type === "offer" && msg.targetPlayerId !== undefined) {
        if (!room.clientQueues.has(msg.targetPlayerId)) {
            room.clientQueues.set(msg.targetPlayerId, []);
        }
        room.clientQueues.get(msg.targetPlayerId).push({ type: "offer", sdp: msg.sdp });
        const resolver = room.clientResolvers.get(msg.targetPlayerId);
        if (resolver) {
            clearTimeout(resolver.timer);
            resolver.resolve(room.clientQueues.get(msg.targetPlayerId));
            room.clientQueues.set(msg.targetPlayerId, []);
            room.clientResolvers.delete(msg.targetPlayerId);
        }
    } else if (msg.type === "answer") {
        room.hostQueue.push({ type: "answer", sdp: msg.sdp, playerId: msg.playerId });
        if (room.hostResolver) {
            clearTimeout(room.hostResolver.timer);
            room.hostResolver.resolve(room.hostQueue);
            room.hostQueue = [];
            room.hostResolver = null;
        }
    } else if (msg.type === "client_joined") {
        room.hostQueue.push({
            type: "client_joined",
            playerId: msg.playerId,
            playerName: msg.playerName,
        });
        if (room.hostResolver) {
            clearTimeout(room.hostResolver.timer);
            room.hostResolver.resolve(room.hostQueue);
            room.hostQueue = [];
            room.hostResolver = null;
        }
    }
    
    cleanupSigRoom(code);
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
        const room = sigRooms.get(code);
        if (room) {
            if (room.hostResolver) {
                clearTimeout(room.hostResolver.timer);
                room.hostResolver.resolve([]);
            }
            for (const resolver of room.clientResolvers.values()) {
                clearTimeout(resolver.timer);
                resolver.resolve([]);
            }
            sigRooms.delete(code);
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

        const joinMsg = { type: "client_joined", playerId, playerName };
        relayToLocal(code, joinMsg);
        bc.postMessage({ roomCode: code, ...joinMsg });

        return json({ playerId, hostName });
    }

    // ── GET /rooms/:code/signal — Long poll endpoint ─────────
    if (action === "signal" && req.method === "GET") {
        const role = url.searchParams.get("role");
        const playerId = Number(url.searchParams.get("playerId") || "0");
        const room = getSigRoom(code);

        if (role === "host") {
            if (room.hostQueue.length > 0) {
                const messages = room.hostQueue;
                room.hostQueue = [];
                cleanupSigRoom(code);
                return json(messages);
            }
            return new Promise((resolve) => {
                const timer = setTimeout(() => {
                    room.hostResolver = null;
                    cleanupSigRoom(code);
                    resolve(json([]));
                }, 25000);
                room.hostResolver = {
                    resolve: (msgs) => resolve(json(msgs)),
                    timer
                };
            });
        } else {
            const queue = room.clientQueues.get(playerId) || [];
            if (queue.length > 0) {
                room.clientQueues.set(playerId, []);
                cleanupSigRoom(code);
                return json(queue);
            }
            return new Promise((resolve) => {
                const timer = setTimeout(() => {
                    room.clientResolvers.delete(playerId);
                    cleanupSigRoom(code);
                    resolve(json([]));
                }, 25000);
                room.clientResolvers.set(playerId, {
                    resolve: (msgs) => resolve(json(msgs)),
                    timer
                });
            });
        }
    }

    // ── POST /rooms/:code/signal — Send signaling message ────
    if (action === "signal" && req.method === "POST") {
        const msg = await req.json();
        relayToLocal(code, msg);
        bc.postMessage({ roomCode: code, ...msg });
        return json({ ok: true });
    }

    return json({ error: "not found" }, 404);
});
