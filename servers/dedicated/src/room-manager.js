const fs = require('fs');
const path = require('path');
const { Room } = require('./room');
const saveReader = require('./save-reader');
class RoomManager {
    constructor() {
        this.rooms = new Map(); 
        this.autoSaveInterval = 5 * 60 * 1000; 
        this.saveTimer = null;
        this.signalingTimer = null;
        this.signalingUrl = 'https://dewy-flea-9364.d050.deno.net';
        this.publicListing = false;
        this.port = 7575;
    }
    start(port, publicListing) {
        this.port = port;
        this.publicListing = publicListing;
        const levelsDir = path.join(process.cwd(), 'levels');
        if (!fs.existsSync(levelsDir)) {
            fs.mkdirSync(levelsDir, { recursive: true });
        }
        this.saveTimer = setInterval(() => this._performAutoSave(), this.autoSaveInterval);
        if (this.publicListing) {
            this._updateSignalingServer();
            this.signalingTimer = setInterval(() => this._updateSignalingServer(), 60 * 1000); 
        }
    }
    stop() {
        clearInterval(this.saveTimer);
        clearInterval(this.signalingTimer);
        for (const room of this.rooms.values()) {
            room.destroy();
        }
        this.rooms.clear();
    }
    createRoom(levelName, levelData, settings) {
        const room = new Room(levelName, levelData, settings);
        room.onSnapshotSaved = (r) => {
            const suffix = r.isAutoSaving ? '_autosave' : '_save';
            r.isAutoSaving = false;
            this._saveRoomToDisk(r, suffix);
        };
        this.rooms.set(room.code, room);
        return room;
    }
    getRoom(code) {
        return this.rooms.get(code.toUpperCase());
    }
    getRoomList() {
        const list = [];
        for (const room of this.rooms.values()) {
            list.push({
                roomCode: room.code,
                roomName: room.levelName,
                playerCount: room.players.size,
                playerLimit: room.maxPlayers,
                isPrivate: room.isPrivate,
                hasPassword: !!room.password
            });
        }
        return list;
    }
    _performAutoSave() {
        for (const room of this.rooms.values()) {
            if (room.players.size > 0 && room.dirty) {
                room.isAutoSaving = true;
                room.requestSnapshot();
            } else if (!room.dirty && room.compressedLevelData && room.compressedLevelData.length > 0) {
                this._saveRoomToDisk(room, '_autosave');
            }
        }
    }
    _saveRoomToDisk(room, suffix = '_autosave') {
        try {
            const levelsDir = path.join(process.cwd(), 'levels');
            const outFile = saveReader.exportToGmd(room, levelsDir, suffix);
            console.log(`  \x1b[35m[SAVE]\x1b[0m Saved level to ${path.basename(outFile)}`);
        } catch (e) {
            console.error(`  \x1b[31m[ERROR]\x1b[0m Failed to save level:`, e.message);
        }
    }
    async _updateSignalingServer() {
        for (const room of this.rooms.values()) {
            if (room.isPrivate) continue;
            try {
                const body = JSON.stringify({
                    hostName: "Dedicated Server",
                    playerName: "Dedicated Server",
                    roomName: room.levelName,
                    description: `Join at ws://<YOUR_IP>:${this.port}`,
                    playerLimit: room.maxPlayers,
                    isPrivate: false,
                    hasPassword: !!room.password,
                    version: "Dedicated"
                });
            } catch (e) {
            }
        }
    }
}
module.exports = { RoomManager };
