const express = require('express');
const http = require('http');
const WebSocket = require('ws');
const crypto = require('crypto');

const app = express();
const server = http.createServer(app);

// WebSocket server configuration
// Render.com handles TLS termination, so we only need plain WebSocket server
const wss = new WebSocket.Server({ 
    server,
    // Handle both ws:// and wss:// (Render terminates TLS for us)
    perMessageDeflate: false,
    maxPayload: 52428800 // 50 MB limit
});

const PORT = process.env.PORT || 8765;
const HOST = process.env.HOST || '0.0.0.0';

// ============================================================
// Room Management
// ============================================================

class Room {
    constructor(hostWs, hostName) {
        this.code = Room.generateCode();
        this.players = new Map();
        this.nextPlayerId = 1;
        this.hostId = this.addPlayer(hostWs, hostName);
        this.createdAt = Date.now();
    }

    static generateCode() {
        // Generate a 6-character alphanumeric code
        return crypto.randomBytes(3).toString('hex').toUpperCase();
    }

    addPlayer(ws, name) {
        const id = this.nextPlayerId++;
        const colorIndex = this.players.size;
        
        this.players.set(id, {
            ws,
            name,
            id,
            colorIndex,
            cursorX: 0,
            cursorY: 0
        });

        if (ws) {
            ws._playerId = id;
            ws._roomCode = this.code;
        }

        return id;
    }

    removePlayer(playerId) {
        this.players.delete(playerId);
    }

    getPlayerList() {
        const list = [];
        for (const [id, player] of this.players) {
            list.push({
                id: player.id,
                name: player.name,
                colorIndex: player.colorIndex,
                status: player.status || ""
            });
        }
        return list;
    }

    broadcast(message, excludePlayerId = null) {
        const raw = JSON.stringify(message);
        // Snapshot the player list: ws.send() can trigger a synchronous
        // close/error callback that mutates this.players mid-iteration, which
        // would otherwise throw and drop the broadcast (a source of "random
        // connection drops" and missed messages).
        for (const player of [...this.players.values()]) {
            if (player.id !== excludePlayerId && player.ws.readyState === WebSocket.OPEN) {
                sendSafe(player.ws, raw);
            }
        }
    }

    sendTo(playerId, message) {
        const player = this.players.get(playerId);
        if (player && player.ws.readyState === WebSocket.OPEN) {
            sendSafe(player.ws, JSON.stringify(message));
        }
    }

    get isEmpty() {
        return this.players.size === 0;
    }

    get playerCount() {
        return this.players.size;
    }
}

// Active rooms: code → Room
const rooms = new Map();

// ============================================================
// HTTP Endpoints (for polling fallback)
// ============================================================

app.use(express.json({ limit: '50mb' }));

// Health check
app.get('/', (req, res) => {
    res.json({
        name: 'Multiplayer Edit Server',
        version: '0.2.0',
        rooms: rooms.size,
        connections: wss.clients.size
    });
});

// Polling endpoints for clients that can't use WebSocket directly
const pollQueues = new Map(); // clientId → message[]

app.post('/send', (req, res) => {
    // Process message from HTTP client
    const message = req.body;
    const clientId = req.headers['x-client-id'] || 'unknown';
    
    handleMessage(null, message, clientId);
    
    // Return any pending messages for this client
    const queue = pollQueues.get(clientId) || [];
    pollQueues.set(clientId, []);
    res.json(queue);
});

app.get('/poll', (req, res) => {
    const clientId = req.headers['x-client-id'] || 'unknown';
    const queue = pollQueues.get(clientId) || [];
    pollQueues.set(clientId, []);
    res.json(queue);
});

// ============================================================
// WebSocket Handler
// ============================================================

wss.on('connection', (ws) => {
    console.log(`[WS] New connection`);

    ws.isAlive = true;
    ws.on('pong', () => {
        ws.isAlive = true;
    });

    // Handshake timeout — if no valid message within 10s, terminate the socket.
    // This prevents zombie half-open connections from accumulating.
    ws._handshakeTimeout = setTimeout(() => {
        if (!ws._playerId) {
            console.log(`[WS] Handshake timeout, closing unregistered connection`);
            ws.terminate();
        }
    }, 10000);

    ws.on('message', (raw) => {
        try {
            // Clear handshake timeout on first valid message
            if (ws._handshakeTimeout) {
                clearTimeout(ws._handshakeTimeout);
                ws._handshakeTimeout = null;
            }

            const rawStr = raw.toString();

            // Fast-extract the action field without parsing the entire JSON.
            // Full JSON.parse on multi-MB level syncs blocks the event loop
            // for seconds, killing new connection handshakes.
            const actionMatch = rawStr.match(/"action"\s*:\s*"([^"]+)"/);
            if (!actionMatch) {
                ws.send(JSON.stringify({ event: 'error', message: 'Missing action field' }));
                return;
            }

            const action = actionMatch[1];

            // Small control messages: full JSON parse (these are always tiny)
            if (action === 'host' || action === 'join' || action === 'leave') {
                const message = JSON.parse(rawStr);
                handleMessage(ws, message);
            } else {
                // Relay messages: raw string pass-through to avoid blocking
                handleRelayRaw(ws, action, rawStr);
            }
        } catch (e) {
            console.error(`[WS] Failed to process message:`, e.message);
            ws.send(JSON.stringify({ event: 'error', message: 'Invalid message' }));
        }
    });

    ws.on('close', () => {
        // Clear handshake timeout if connection closes early
        if (ws._handshakeTimeout) {
            clearTimeout(ws._handshakeTimeout);
            ws._handshakeTimeout = null;
        }
        // Only process disconnect for sockets that completed the handshake
        if (ws._playerId) {
            console.log(`[WS] Connection closed (player=${ws._playerId})`);
            handleDisconnect(ws);
        } else {
            console.log(`[WS] Unregistered connection closed (no handshake completed)`);
        }
    });

    ws.on('error', (err) => {
        console.error(`[WS] Error:`, err.message);
    });
});

// ============================================================
// Message Handler
// ============================================================

function handleMessage(ws, message, httpClientId = null) {
    const action = message.action;

    switch (action) {
        case 'host':
            handleHost(ws, message, httpClientId);
            break;

        case 'join':
            handleJoin(ws, message, httpClientId);
            break;

        case 'leave':
            handleLeave(ws);
            break;

        default:
            // Relay actions are handled via handleRelayRaw for WebSocket clients.
            // HTTP fallback clients can only host/join, so unknown actions are ignored.
            if (ws) {
                sendError(ws, `Unknown action: ${action}`);
            }
    }
}

function handleHost(ws, message, httpClientId) {
    const playerName = message.playerName || 'Host';
    const room = new Room(ws, playerName);
    rooms.set(room.code, room);

    const response = {
        event: 'room_created',
        roomCode: room.code,
        playerId: room.hostId
    };

    if (ws) {
        ws.send(JSON.stringify(response));
    } else if (httpClientId) {
        const queue = pollQueues.get(httpClientId) || [];
        queue.push(response);
        pollQueues.set(httpClientId, queue);
    }

    console.log(`[Room] Created room ${room.code} by ${playerName}`);
}

function handleJoin(ws, message, httpClientId) {
    const roomCode = message.roomCode?.toUpperCase();
    const playerName = message.playerName || 'Player';

    if (!roomCode) {
        sendError(ws, 'Room code is required');
        return;
    }

    const room = rooms.get(roomCode);
    if (!room) {
        sendError(ws, `Room '${roomCode}' not found`);
        return;
    }

    if (room.playerCount >= 8) {
        sendError(ws, 'Room is full (max 8 players)');
        return;
    }

    const playerId = room.addPlayer(ws, playerName);

    // Send join confirmation to the new player
    const joinResponse = {
        event: 'room_joined',
        roomCode: room.code,
        playerId: playerId,
        players: room.getPlayerList()
    };

    if (ws) {
        ws.send(JSON.stringify(joinResponse));
    }

    // Notify existing players
    room.broadcast({
        event: 'player_joined',
        playerId: playerId,
        playerName: playerName,
        colorIndex: room.players.get(playerId)?.colorIndex || 0
    }, playerId);

    console.log(`[Room] ${playerName} joined room ${roomCode} (${room.playerCount} players)`);
}

function handleLeave(ws) {
    if (!ws || !ws._roomCode) return;

    const room = rooms.get(ws._roomCode);
    if (!room) return;

    const playerId = ws._playerId;

    if (playerId === room.hostId) {
        room.broadcast({
            event: 'error',
            message: 'The host has left the session.'
        }, playerId);
        rooms.delete(ws._roomCode);
        console.log(`[Room] Host left, room ${ws._roomCode} deleted`);
        return;
    }

    room.removePlayer(playerId);

    // Notify remaining players
    room.broadcast({
        event: 'player_left',
        playerId: playerId
    });

    // Clean up empty rooms
    if (room.isEmpty) {
        rooms.delete(ws._roomCode);
        console.log(`[Room] Room ${ws._roomCode} deleted (empty)`);
    }

    console.log(`[Room] Player ${playerId} left room ${ws._roomCode}`);
}

// ============================================================
// Raw Relay Handler (avoids event-loop-blocking JSON round-trips)
// ============================================================

// Action type → event type mapping
const ACTION_TO_EVENT = {
    'place_objects': 'objects_placed',
    'delete_objects': 'objects_deleted',
    'move_objects': 'objects_moved',
    'transform_objects': 'objects_transformed',
    'update_objects': 'update_objects',
    'lock_objects': 'lock_objects',
    'sync_level': 'sync_level',
    'update_settings': 'update_settings',
    'cursor_update': 'cursor_moved'
};

function handleRelayRaw(ws, action, rawStr) {
    if (!ws || !ws._roomCode) return;

    const room = rooms.get(ws._roomCode);
    if (!room) return;

    const eventType = ACTION_TO_EVENT[action];
    if (!eventType) {
        sendError(ws, `Unknown action: ${action}`);
        return;
    }

    // Replace "action":"xxx" with "event":"yyy","playerId":N directly in the
    // raw string. This avoids the expensive JSON.parse → object spread →
    // JSON.stringify cycle that was blocking the event loop on multi-MB syncs.
    const outgoing = rawStr.replace(
        /"action"\s*:\s*"[^"]+"/,
        `"event":"${eventType}","playerId":${ws._playerId}`
    );

    if (action === 'sync_level') {
        // Directed sync: extract targetPlayerId and send only to that player
        const targetMatch = rawStr.match(/"targetPlayerId"\s*:\s*(\d+)/);
        if (targetMatch) {
            const targetId = parseInt(targetMatch[1]);
            const target = room.players.get(targetId);
            sendSafe(target && target.ws, outgoing);
        } else {
            broadcastRaw(room, outgoing, ws._playerId);
        }
    } else {
        broadcastRaw(room, outgoing, ws._playerId);
    }
}

function broadcastRaw(room, rawStr, excludePlayerId) {
    // Snapshot the values for the same reason as Room.broadcast: a send that
    // triggers a close handler must not mutate the map mid-iteration.
    for (const player of [...room.players.values()]) {
        if (player.id !== excludePlayerId && player.ws.readyState === WebSocket.OPEN) {
            sendSafe(player.ws, rawStr);
        }
    }
}

function handleDisconnect(ws) {
    handleLeave(ws);
}

// ws.send() can throw if the socket dies between the readyState check and the
// call (a race on flaky/mobile connections). Wrapping it keeps one bad socket
// from aborting a broadcast to everyone else.
function sendSafe(ws, raw) {
    try {
        if (ws && ws.readyState === WebSocket.OPEN) {
            ws.send(raw);
        }
    } catch (e) {
        console.error(`[WS] send failed: ${e.message}`);
    }
}

function sendError(ws, msg) {
    sendSafe(ws, JSON.stringify({ event: 'error', message: msg }));
    console.log(`[Error] ${msg}`);
}

// ============================================================
// Room Cleanup (remove stale rooms every 5 minutes)
// ============================================================

setInterval(() => {
    const now = Date.now();
    const staleTimeout = 30 * 60 * 1000; // 30 minutes

    for (const [code, room] of rooms) {
        if (room.isEmpty || (now - room.createdAt > staleTimeout && room.playerCount === 0)) {
            rooms.delete(code);
            console.log(`[Cleanup] Removed stale room ${code}`);
        }
    }
}, 5 * 60 * 1000);

// ============================================================
// Broken Connection Detection (Heartbeat)
// ============================================================

const heartbeatInterval = setInterval(() => {
    wss.clients.forEach((ws) => {
        if (ws.isAlive === false) {
            console.log(`[WS] Connection timed out, terminating (player=${ws._playerId || 'unregistered'})`);
            return ws.terminate();
        }
        ws.isAlive = false;
        ws.ping();
    });
}, 30000);

wss.on('close', () => {
    clearInterval(heartbeatInterval);
});

// ============================================================
// Start Server
// ============================================================

server.listen(PORT, HOST, () => {
    console.log(`========================================`);
    console.log(`  Multiplayer Edit Server v0.2.0`);
    console.log(`  Listening on ${HOST}:${PORT}`);
    console.log(`  WebSocket: ws://${HOST}:${PORT}`);
    console.log(`  HTTP:      http://${HOST}:${PORT}`);
    console.log(`========================================`);
});
