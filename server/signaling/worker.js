// Signaling server for WebRTC P2P connections (Deno Deploy)
// Room metadata and signaling queues use Deno.Kv.
// BroadcastChannel is used as a fast wake-up signal across isolates in the same region.

const kv = await Deno.openKv();
const ROOM_TTL = 2 * 60 * 60 * 1000; // 2 hours
const CHARS = "ABCDEFGHJKMNPQRSTUVWXYZ23456789"; // no 0/O/1/I/L

// ── In-memory Wakeup State (Zero signaling data stored here) ──
// code -> { hostResolver: Function|null, clientResolvers: Map<playerId, Function> }
const sigRooms = new Map();

function getSigRoom(code) {
    if (!sigRooms.has(code)) {
        sigRooms.set(code, { hostResolver: null, clientResolvers: new Map() });
    }
    return sigRooms.get(code);
}

function cleanupSigRoom(code) {
    const room = sigRooms.get(code);
    if (!room) return;
    if (!room.hostResolver && room.clientResolvers.size === 0) {
        sigRooms.delete(code);
    }
}

// ── Cross-isolate relay via BroadcastChannel ─────────────────
const bc = new BroadcastChannel("signaling_wake");
bc.onmessage = (e) => {
    const { roomCode, target } = e.data; // target: "host" or playerId
    const room = sigRooms.get(roomCode);
    if (!room) return;

    if (target === "host" && room.hostResolver) {
        room.hostResolver();
    } else if (typeof target === "number" && room.clientResolvers.has(target)) {
        room.clientResolvers.get(target)();
    }
};

function wakeLocalAndBroadcast(code, target) {
    const room = sigRooms.get(code);
    if (room) {
        if (target === "host" && room.hostResolver) {
            room.hostResolver();
        } else if (typeof target === "number" && room.clientResolvers.has(target)) {
            room.clientResolvers.get(target)();
        }
    }
    bc.postMessage({ roomCode: code, target });
}

// ── KV Queue Helpers ─────────────────────────────────────────

async function enqueueKv(code, queueName, msg) {
    // Give each message a unique, sortable key to prevent write conflicts
    const msgId = Date.now() + "_" + crypto.randomUUID();
    await kv.set(["rooms", code, queueName, msgId], msg, { expireIn: ROOM_TTL });
}

async function dequeueKv(code, queueName) {
    const prefix = ["rooms", code, queueName];
    const msgs = [];
    const entriesToDelete = [];
    
    // Fetch all messages in the queue
    for await (const entry of kv.list({ prefix })) {
        msgs.push(entry.value);
        entriesToDelete.push(entry);
    }
    
    if (msgs.length === 0) return [];

    // Delete them in atomic batches (Deno KV allows max 10 mutations per atomic block)
    let atomic = kv.atomic();
    let ops = 0;
    for (const entry of entriesToDelete) {
        atomic = atomic.check(entry).delete(entry.key);
        ops++;
        if (ops === 10) {
            await atomic.commit();
            atomic = kv.atomic();
            ops = 0;
        }
    }
    if (ops > 0) {
        await atomic.commit();
    }
    
    return msgs;
}

// ── Core HTTP ────────────────────────────────────────────────

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

    if (parts[0] === "health" && req.method === "GET") {
        return json({ status: "ok" });
    }

    if (parts[0] !== "rooms") return json({ error: "not found" }, 404);

    // ── GET /rooms — list public rooms
    if (parts.length === 1 && req.method === "GET") {
        const rooms = [];
        for await (const entry of kv.list({ prefix: ["rooms"] })) {
            if (entry.key.length === 2) {
                const room = entry.value;
                if (!room.isPrivate && room.version && room.version !== "Unknown") {
                    rooms.push({
                        roomCode: entry.key[1],
                        hostName: room.hostName,
                        roomName: room.roomName || "Room",
                        description: room.description || "",
                        playerCount: room.players.length,
                        playerLimit: room.playerLimit || 0,
                        isPrivate: !!room.isPrivate,
                        hasPassword: !!room.hasPassword,
                        version: room.version || "Unknown",
                        created: room.created || 0
                    });
                }
            }
        }
        
        rooms.sort((a, b) => b.created - a.created);
        return json(rooms);
    }

    // ── POST /rooms — create room
    if (parts.length === 1 && req.method === "POST") {
        const { hostName, playerName, roomName, description, playerLimit, isPrivate, hasPassword, password, version } = await req.json();
        const code = await genCode();
        const roomId = crypto.randomUUID();
        
        const host = hostName || playerName || "Unknown";

        await kv.set(
            ["rooms", code],
            {
                roomId,
                hostName: host,
                roomName: roomName || "Room",
                description: description || "",
                playerLimit: playerLimit || 0,
                isPrivate: !!isPrivate,
                hasPassword: !!hasPassword,
                password: password || "",
                version: version || "Unknown",
                nextId: 1,
                created: Date.now(),
                players: [{ id: 0, name: host }],
            },
            { expireIn: ROOM_TTL }
        );

        return json({ roomCode: code, roomId });
    }

    const code = parts[1]?.toUpperCase();
    const action = parts[2];

    // ── GET /rooms/:code — room info
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

    // ── DELETE /rooms/:code — close room
    if (parts.length === 2 && req.method === "DELETE") {
        await kv.delete(["rooms", code]);
        wakeLocalAndBroadcast(code, "host");
        // We can't cleanly wake all clients across all isolates without iterating,
        // but they will timeout anyway, which is fine for delete.
        return json({ ok: true });
    }

    // ── POST /rooms/:code/join
    if (action === "join" && req.method === "POST") {
        const { playerName, password } = await req.json();

        let success = false;
        let playerId = -1;
        let retries = 5;
        let hostName = "";

        while (!success && retries > 0) {
            const currentRes = await kv.get(["rooms", code]);
            const currentRoom = currentRes.value;
            if (!currentRoom) return json({ error: "room not found" }, 404);
            
            if (currentRoom.hasPassword && currentRoom.password !== password)
                return json({ error: "invalid password" }, 403);
                
            if (currentRoom.banned && currentRoom.banned.includes(playerName))
                return json({ error: "you are banned" }, 403);
                
            if (currentRoom.playerLimit > 0 && currentRoom.players.length >= currentRoom.playerLimit)
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
        await enqueueKv(code, "hostQueue", joinMsg);
        wakeLocalAndBroadcast(code, "host");

        return json({ playerId, hostName });
    }

    // ── POST /rooms/:code/ban
    if (action === "ban" && req.method === "POST") {
        const { playerName } = await req.json();

        let success = false;
        let retries = 5;

        while (!success && retries > 0) {
            const currentRes = await kv.get(["rooms", code]);
            const currentRoom = currentRes.value;
            if (!currentRoom) return json({ error: "room not found" }, 404);

            if (!currentRoom.banned) currentRoom.banned = [];
            if (!currentRoom.banned.includes(playerName)) {
                currentRoom.banned.push(playerName);
            }

            const commit = await kv
                .atomic()
                .check(currentRes)
                .set(["rooms", code], currentRoom, { expireIn: ROOM_TTL })
                .commit();

            success = commit.ok;
            retries--;
        }
        if (!success) return json({ error: "concurrent ban failed" }, 500);
        return json({ ok: true });
    }

    // ── POST /rooms/:code/leave
    if (action === "leave" && req.method === "POST") {
        const { playerId } = await req.json();

        let success = false;
        let retries = 5;

        while (!success && retries > 0) {
            const currentRes = await kv.get(["rooms", code]);
            const currentRoom = currentRes.value;
            if (!currentRoom) return json({ error: "room not found" }, 404);

            currentRoom.players = currentRoom.players.filter(p => p.id !== playerId);

            const commit = await kv
                .atomic()
                .check(currentRes)
                .set(["rooms", code], currentRoom, { expireIn: ROOM_TTL })
                .commit();

            success = commit.ok;
            retries--;
        }
        return json({ ok: success });
    }

    // ── GET /rooms/:code/signal — Long poll endpoint
    if (action === "signal" && req.method === "GET") {
        const role = url.searchParams.get("role");
        const playerId = Number(url.searchParams.get("playerId") || "0");
        const queueName = role === "host" ? "hostQueue" : `clientQueue_${playerId}`;
        const target = role === "host" ? "host" : playerId;
        const room = getSigRoom(code);

        // Check KV immediately
        const initialMsgs = await dequeueKv(code, queueName);
        if (initialMsgs.length > 0) return json(initialMsgs);

        return new Promise((resolve) => {
            let isResolved = false;

            const complete = async (overrideMsgs) => {
                if (isResolved) return;
                isResolved = true;
                clearTimeout(timer);
                
                if (role === "host") room.hostResolver = null;
                else room.clientResolvers.delete(playerId);
                cleanupSigRoom(code);

                const msgs = overrideMsgs || await dequeueKv(code, queueName);
                resolve(json(msgs));
            };

            // Wake up on BroadcastChannel (cross-isolate) or local wake
            if (role === "host") room.hostResolver = complete;
            else room.clientResolvers.set(playerId, complete);

            // Timeout after 25 seconds — client will re-poll, which
            // gives us a natural KV check every 25s as a fallback.
            const timer = setTimeout(() => {
                if (isResolved) return;
                isResolved = true;
                
                if (role === "host") room.hostResolver = null;
                else room.clientResolvers.delete(playerId);
                cleanupSigRoom(code);
                
                resolve(json([])); // resolve empty to restart poll
            }, 25000);
        });
    }

    // ── POST /rooms/:code/signal — Send signaling message
    if (action === "signal" && req.method === "POST") {
        const msg = await req.json();
        let targetQueue = "";
        let targetWake = null;

        if (msg.type === "offer" || (msg.type === "candidate" && msg.targetPlayerId !== undefined)) {
            targetQueue = `clientQueue_${msg.targetPlayerId}`;
            targetWake = msg.targetPlayerId;
        } else if (msg.type === "answer" || (msg.type === "candidate" && msg.playerId !== undefined)) {
            targetQueue = "hostQueue";
            targetWake = "host";
        }

        if (targetQueue) {
            await enqueueKv(code, targetQueue, msg);
            wakeLocalAndBroadcast(code, targetWake);
        }

        return json({ ok: true });
    }

    return json({ error: "not found" }, 404);
});

