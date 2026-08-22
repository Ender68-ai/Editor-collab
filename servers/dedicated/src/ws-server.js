const http = require('http');
const WebSocket = require('ws');
const proto = require('./protocol');
class WSServer {
    constructor(roomManager) {
        this.roomManager = roomManager;
        this.server = http.createServer((req, res) => this._handleHttp(req, res));
        this.server.keepAliveTimeout = 0;
        this.server.headersTimeout = 0;
        this.wss = new WebSocket.Server({ noServer: true });
        this.server.on('upgrade', (request, socket, head) => {
            const url = new URL(request.url, `http://${request.headers.host}`);
            const pathParts = url.pathname.split('/').filter(Boolean);
            let roomCode = null;
            if (pathParts.length > 0) {
                roomCode = pathParts[0].toUpperCase();
            } else if (this.roomManager.rooms.size === 1) {
                roomCode = Array.from(this.roomManager.rooms.keys())[0];
            }
            const room = roomCode ? this.roomManager.getRoom(roomCode) : null;
            if (!room) {
                socket.write('HTTP/1.1 404 Not Found\r\n\r\n');
                socket.destroy();
                return;
            }
            this.wss.handleUpgrade(request, socket, head, (ws) => {
                this.wss.emit('connection', ws, request, room);
            });
        });
        this.wss.on('connection', (ws, request, room) => {
            this._handleConnection(ws, request, room);
        });
    }
    start(port) {
        return new Promise((resolve) => {
            this.server.listen(port, '0.0.0.0', () => {
                resolve();
            });
        });
    }
    stop() {
        this.server.close();
        for (const client of this.wss.clients) {
            client.close();
        }
    }
    _handleHttp(req, res) {
        res.setHeader('Access-Control-Allow-Origin', '*');
        if (req.method === 'GET' && req.url === '/rooms') {
            res.writeHead(200, { 'Content-Type': 'application/json' });
            res.end(JSON.stringify(this.roomManager.getRoomList()));
            return;
        }
        res.writeHead(404);
        res.end('Not Found');
    }
    _handleConnection(ws, request, room) {
        ws.binaryType = 'nodebuffer';
        let playerId = -1;
        ws.on('message', (message) => {
            if (playerId === -1) {
                if (message.length > 0 && message[0] === proto.Opcode.PlayerJoined) {
                    const r = new proto.Reader(message.slice(1));
                    const msg = proto.deserializePlayerJoined(r);
                    if (!r.error) {
                        playerId = room.addPlayer(msg.name, msg.colorIndex, ws, msg.iconStr);
                        if (playerId === null) {
                            ws.close();
                        }
                    } else {
                        ws.close(1008, 'Invalid handshake');
                    }
                } else {
                    ws.close(1008, 'Expected handshake');
                }
                return;
            }
            room.handleMessage(playerId, message);
        });
        ws.on('close', (code, reason) => { console.log(`  [${room.code}] Player ${playerId} socket closed with code ${code}, reason: ${reason}`);
            if (playerId !== -1) {
                room.removePlayer(playerId);
            }
        });
        ws.on('error', (err) => {
            console.error(`  [${room.code}] Connection error for player ${playerId}:`, err.message);
        });
    }
}
module.exports = { WSServer };
