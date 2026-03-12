const WebSocket = require('ws');
const http = require('http');
const fs = require('fs');
const path = require('path');

class CollabServer {
    constructor(port = 8080) {
        this.port = port;
        this.clients = new Map();
        this.sessions = new Map();
        this.setupServer();
    }
    
    setupServer() {
        // Create HTTP server for WebSocket upgrade
        this.server = http.createServer((req, res) => {
            // Handle basic HTTP requests
            if (req.url === '/') {
                res.writeHead(200, { 'Content-Type': 'text/html' });
                res.end(`
                    <!DOCTYPE html>
                    <html>
                    <head>
                        <title>Collaborative Editor Server</title>
                        <style>
                            body { font-family: Arial, sans-serif; margin: 40px; }
                            .stats { background: #f0f0f0; padding: 20px; border-radius: 5px; }
                            .client { margin: 10px 0; padding: 10px; background: white; border-radius: 3px; }
                        </style>
                    </head>
                    <body>
                        <h1>Collaborative Editor Server</h1>
                        <div class="stats">
                            <h2>Server Status</h2>
                            <p>Port: ${this.port}</p>
                            <p>Connected Clients: <span id="clientCount">0</span></p>
                            <p>Active Sessions: <span id="sessionCount">0</span></p>
                        </div>
                        <div id="clients"></div>
                        <script>
                            function updateStats() {
                                fetch('/stats').then(r => r.json()).then(data => {
                                    document.getElementById('clientCount').textContent = data.clients;
                                    document.getElementById('sessionCount').textContent = data.sessions;
                                    
                                    const clientsDiv = document.getElementById('clients');
                                    clientsDiv.innerHTML = '<h2>Connected Clients</h2>' + 
                                        data.clientList.map(client => 
                                            \`<div class="client">
                                                <strong>\${client.playerName}</strong> (\${client.playerID})<br>
                                                Connected: \${new Date(client.connectedAt).toLocaleString()}
                                            </div>\`
                                        ).join('');
                                });
                            }
                            
                            updateStats();
                            setInterval(updateStats, 1000);
                        </script>
                    </body>
                    </html>
                `);
            } else if (req.url === '/stats') {
                res.writeHead(200, { 'Content-Type': 'application/json' });
                const clientList = Array.from(this.clients.values()).map(client => ({
                    playerID: client.playerID,
                    playerName: client.playerName,
                    connectedAt: client.connectedAt
                }));
                res.end(JSON.stringify({
                    clients: this.clients.size,
                    sessions: this.sessions.size,
                    clientList: clientList
                }));
            } else {
                res.writeHead(404);
                res.end('Not Found');
            }
        });
        
        // Setup WebSocket server
        this.wss = new WebSocket.Server({ 
            server: this.server,
            path: '/collab'
        });
        
        this.wss.on('connection', (ws, req) => {
            this.handleConnection(ws, req);
        });
        
        console.log(`Collaborative Editor Server starting on port ${this.port}`);
    }
    
    handleConnection(ws, req) {
        const clientId = this.generateClientId();
        const client = {
            id: clientId,
            ws: ws,
            playerID: null,
            playerName: null,
            color: null,
            connectedAt: new Date(),
            lastPing: Date.now()
        };
        
        this.clients.set(clientId, client);
        
        console.log(`Client connected: ${clientId}`);
        
        ws.on('message', (data) => {
            this.handleMessage(clientId, data);
        });
        
        ws.on('close', () => {
            this.handleDisconnection(clientId);
        });
        
        ws.on('error', (error) => {
            console.error(`WebSocket error for client ${clientId}:`, error);
        });
        
        // Send welcome message
        this.sendToClient(clientId, {
            type: 'WELCOME',
            clientId: clientId
        });
    }
    
    handleMessage(clientId, data) {
        try {
            const message = JSON.parse(data);
            const client = this.clients.get(clientId);
            
            if (!client) return;
            
            switch (message.type) {
                case 'PLAYER_JOIN':
                    this.handlePlayerJoin(clientId, message);
                    break;
                    
                case 'CURSOR_UPDATE':
                    this.handleCursorUpdate(clientId, message);
                    break;
                    
                case 'CHAT_MESSAGE':
                    this.handleChatMessage(clientId, message);
                    break;
                    
                case 'ADD_OBJECT':
                case 'REMOVE_OBJECT':
                case 'MOVE_OBJECT':
                case 'CLAIM_OBJECT':
                case 'TRANSFER_OWNERSHIP':
                    this.handleObjectAction(clientId, message);
                    break;
                    
                case 'REQUEST_SYNC':
                    this.handleSyncRequest(clientId, message);
                    break;
                    
                case 'FULL_SYNC':
                    this.handleFullSync(clientId, message);
                    break;
                    
                default:
                    console.log(`Unknown message type: ${message.type}`);
            }
        } catch (error) {
            console.error(`Error handling message from ${clientId}:`, error);
        }
    }
    
    handlePlayerJoin(clientId, message) {
        const client = this.clients.get(clientId);
        if (!client) return;
        
        client.playerID = message.playerID;
        client.playerName = message.playerName;
        client.color = message.color;
        
        // Broadcast to all other clients
        this.broadcast(clientId, {
            type: 'PLAYER_JOIN',
            playerID: message.playerID,
            playerName: message.playerName,
            color: message.color
        });
        
        // Send current player list to new client
        const players = Array.from(this.clients.values())
            .filter(c => c.id !== clientId && c.playerID)
            .map(c => ({
                type: 'PLAYER_JOIN',
                playerID: c.playerID,
                playerName: c.playerName,
                color: c.color
            }));
        
        players.forEach(player => {
            this.sendToClient(clientId, player);
        });
        
        console.log(`Player joined: ${client.playerName} (${client.playerID})`);
    }
    
    handleCursorUpdate(clientId, message) {
        const client = this.clients.get(clientId);
        if (!client || !client.playerID) return;
        
        // Broadcast cursor position to other clients
        this.broadcast(clientId, {
            type: 'CURSOR_UPDATE',
            playerID: client.playerID,
            x: message.x,
            y: message.y
        });
    }
    
    handleChatMessage(clientId, message) {
        const client = this.clients.get(clientId);
        if (!client || !client.playerID) return;
        
        // Broadcast chat message to all clients
        this.broadcast(null, {
            type: 'CHAT_MESSAGE',
            playerID: client.playerID,
            playerName: client.playerName,
            message: message.message,
            timestamp: Date.now()
        });
    }
    
    handleObjectAction(clientId, message) {
        const client = this.clients.get(clientId);
        if (!client || !client.playerID) return;
        
        // Add player ID to message if not present
        if (!message.playerID) {
            message.playerID = client.playerID;
        }
        
        // Broadcast object action to all other clients
        this.broadcast(clientId, message);
    }
    
    handleSyncRequest(clientId, message) {
        const client = this.clients.get(clientId);
        if (!client) return;
        
        // Request sync from a random connected client (or host)
        const otherClients = Array.from(this.clients.values())
            .filter(c => c.id !== clientId && c.playerID);
        
        if (otherClients.length > 0) {
            const targetClient = otherClients[0];
            this.sendToClient(targetClient.id, {
                type: 'SYNC_REQUEST',
                requestingPlayerID: client.playerID
            });
        }
    }
    
    handleFullSync(clientId, message) {
        const client = this.clients.get(clientId);
        if (!client) return;
        
        // Forward sync data to requesting player
        const requestingPlayerID = message.requestingPlayerID;
        const targetClient = Array.from(this.clients.values())
            .find(c => c.playerID === requestingPlayerID);
        
        if (targetClient) {
            this.sendToClient(targetClient.id, {
                type: 'FULL_SYNC',
                playerID: client.playerID,
                levelData: message.levelData
            });
        }
    }
    
    handleDisconnection(clientId) {
        const client = this.clients.get(clientId);
        if (!client) return;
        
        if (client.playerID) {
            // Broadcast player leave
            this.broadcast(clientId, {
                type: 'PLAYER_LEAVE',
                playerID: client.playerID
            });
            
            console.log(`Player left: ${client.playerName} (${client.playerID})`);
        }
        
        this.clients.delete(clientId);
        console.log(`Client disconnected: ${clientId}`);
    }
    
    sendToClient(clientId, message) {
        const client = this.clients.get(clientId);
        if (!client || client.ws.readyState !== WebSocket.OPEN) {
            return;
        }
        
        try {
            client.ws.send(JSON.stringify(message));
        } catch (error) {
            console.error(`Error sending message to ${clientId}:`, error);
        }
    }
    
    broadcast(excludeClientId, message) {
        this.clients.forEach((client, clientId) => {
            if (clientId !== excludeClientId && client.ws.readyState === WebSocket.OPEN) {
                this.sendToClient(clientId, message);
            }
        });
    }
    
    generateClientId() {
        return Math.random().toString(36).substr(2, 9);
    }
    
    start() {
        this.server.listen(this.port, () => {
            console.log(`Collaborative Editor Server running on port ${this.port}`);
            console.log(`WebSocket endpoint: ws://localhost:${this.port}/collab`);
            console.log(`Status page: http://localhost:${this.port}/`);
        });
    }
    
    stop() {
        this.wss.close();
        this.server.close();
        console.log('Server stopped');
    }
}

// Start server if run directly
if (require.main === module) {
    const port = process.argv[2] ? parseInt(process.argv[2]) : 8080;
    const server = new CollabServer(port);
    
    // Handle graceful shutdown
    process.on('SIGINT', () => {
        console.log('\nShutting down server...');
        server.stop();
        process.exit(0);
    });
    
    server.start();
}

module.exports = CollabServer;
