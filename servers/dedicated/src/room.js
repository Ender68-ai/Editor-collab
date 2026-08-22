const zlib = require('zlib');
const proto = require('./protocol');
const CHARS = 'ABCDEFGHJKMNPQRSTUVWXYZ23456789';
const MAX_CHUNK_BYTES = 30000;
const MAX_UUIDS_PER_CHUNK = 500;
function generateRoomCode() {
    let code = '';
    for (let i = 0; i < 6; i++) {
        code += CHARS[Math.floor(Math.random() * CHARS.length)];
    }
    return code;
}
let uuidCounter = 0;
function generateUUID() {
    const now = Date.now();
    const rand = Math.floor(Math.random() * 0xFFFF);
    return `s${now.toString(16)}${(uuidCounter++).toString(16)}${rand.toString(16)}`;
}
class Room {
    constructor(levelName, levelData, settings) {
        this.code = generateRoomCode();
        this.levelName = levelName;
        this.compressedLevelData = levelData.compressedBytes; 
        this.uuids = levelData.uuids || [];
        this.settings = settings || { saveString: '', audioTrack: 0, songID: 0, levelLength: 0 };
        this.locks = new Map(); 
        this.colorChannels = new Map(); 
        this.players = new Map(); 
        this.nextPlayerId = 1; 
        this.banned = new Set();
        this.password = '';
        this.maxPlayers = 100;
        this.isPrivate = false;
        this.dirty = false;
        this.snapshotPending = false;
        this.snapshotState = null; 
        this.lockDecayInterval = setInterval(() => this._decayLocks(), 1000);
    }
    destroy() {
        clearInterval(this.lockDecayInterval);
        for (const [, player] of this.players) {
            if (player.ws && player.ws.readyState === 1) {
                player.ws.close(1001, 'Room closed');
            }
        }
        this.players.clear();
    }
    addPlayer(name, colorIndex, ws, iconStr = "") {
        if (this.banned.has(name)) {
            const errBuf = proto.serializeError('You are banned from this room');
            ws.send(errBuf);
            ws.close(1008, 'Banned');
            return null;
        }
        if (this.password && !ws._authed) {
        }
        if (this.maxPlayers > 0 && this.players.size >= this.maxPlayers) {
            const errBuf = proto.serializeError('Room is full');
            ws.send(errBuf);
            ws.close(1008, 'Room full');
            return null;
        }
        const playerId = this.nextPlayerId++;
        const player = { id: playerId, name, colorIndex, iconStr, ws };
        this.players.set(playerId, player);
        console.log(`  [${this.code}] ${name} joined (id: ${playerId}, ${this.players.size} players)`);
        const joinMsg = proto.serializePlayerJoined(playerId, name, colorIndex, iconStr);
        this._broadcastExcept(joinMsg, playerId);
        const roomPlayers = [];
        for (const [, p] of this.players) {
            roomPlayers.push({ id: p.id, name: p.name, colorIndex: p.colorIndex, iconStr: p.iconStr });
        }
        ws.send(proto.serializeRoomInfo(playerId, roomPlayers));
        for (const [, p] of this.players) {
            if (p.id === playerId) continue;
            ws.send(proto.serializePlayerJoined(p.id, p.name, p.colorIndex, p.iconStr));
        }
        this._syncLevelTo(ws);
        return playerId;
    }
    removePlayer(playerId) {
        const player = this.players.get(playerId);
        if (!player) return;
        this.players.delete(playerId);
        console.log(`  [${this.code}] ${player.name} left (${this.players.size} players)`);
        for (const [uuid, lock] of this.locks) {
            if (lock.playerId === playerId) {
                this.locks.delete(uuid);
            }
        }
        const leftMsg = proto.serializePlayerLeft(playerId);
        this._broadcastExcept(leftMsg, playerId);
    }
    handleMessage(playerId, data) {
        if (data.length === 0) return;
        const opcode = data[0];
        if (opcode === proto.Opcode.Heartbeat) return;
        
        if (opcode === proto.Opcode.SyncLevelStart || opcode === proto.Opcode.SyncLevelChunk || opcode === proto.Opcode.SyncLevelEnd) {
            this._handleSnapshotResponse(playerId, opcode, data);
            return;
        }
        if (opcode === proto.Opcode.CursorUpdate || opcode === proto.Opcode.MoveBatch) {
            this._relayFrom(playerId, data);
            this.dirty = true;
            return;
        }
        switch (opcode) {
            case proto.Opcode.PlaceObjects:
                this._onPlaceObjects(playerId, data);
                break;
            case proto.Opcode.DeleteObjects:
                this._onDeleteObjects(playerId, data);
                break;
            case proto.Opcode.MoveObjects:
            case proto.Opcode.TransformObjects:
            case proto.Opcode.UpdateObjects:
            case proto.Opcode.ReconcileObjects:
                this._relayFrom(playerId, data);
                this.dirty = true;
                break;
            case proto.Opcode.LockObjects:
                this._onLockObjects(playerId, data);
                break;
            case proto.Opcode.UpdateSettings:
                this._onUpdateSettings(playerId, data);
                break;
            case proto.Opcode.UpdateColorChannel:
                this._onUpdateColorChannel(playerId, data);
                break;
            case proto.Opcode.KickPlayer:
                this._onKickPlayer(playerId, data);
                break;
            case proto.Opcode.BanPlayer:
                this._onBanPlayer(playerId, data);
                break;
            case proto.Opcode.SetViewOnly:
                this._relayFrom(playerId, data);
                break;
            default:
                this._relayFrom(playerId, data);
                break;
        }
    }
    _onPlaceObjects(fromId, data) {
        const r = new proto.Reader(data.slice(1));
        const msg = proto.deserializePlaceObjects(r);
        if (r.error) return;
        for (const obj of msg.objects) {
            if (obj.uuid && !this.uuids.includes(obj.uuid)) {
                this.uuids.push(obj.uuid);
            }
        }
        this._relayFrom(fromId, data);
        this.dirty = true;
    }
    _onDeleteObjects(fromId, data) {
        const r = new proto.Reader(data.slice(1));
        const msg = proto.deserializeDeleteObjects(r);
        if (r.error) return;
        const toDelete = new Set(msg.uuids);
        this.uuids = this.uuids.filter(u => !toDelete.has(u));
        for (const uuid of msg.uuids) {
            this.locks.delete(uuid);
        }
        this._relayFrom(fromId, data);
        this.dirty = true;
    }
    _onLockObjects(fromId, data) {
        const r = new proto.Reader(data.slice(1));
        const msg = proto.deserializeLockObjects(r);
        if (r.error) return;
        if (msg.locked) {
            for (const uuid of msg.uuids) {
                this.locks.set(uuid, { playerId: fromId, expiresAt: Date.now() + 5000 });
            }
        } else {
            for (const uuid of msg.uuids) {
                const existing = this.locks.get(uuid);
                if (existing && existing.playerId === fromId) {
                    this.locks.delete(uuid);
                }
            }
        }
        this._relayFrom(fromId, data);
    }
    _onUpdateSettings(fromId, data) {
        const r = new proto.Reader(data.slice(1));
        const msg = proto.deserializeUpdateSettings(r);
        if (r.error) return;
        this.settings = msg.settings;
        this._relayFrom(fromId, data);
        this.dirty = true;
    }
    _onUpdateColorChannel(fromId, data) {
        const r = new proto.Reader(data.slice(1));
        const channelID = r.readU32();
        if (!r.error) {
            this.colorChannels.set(channelID, Buffer.from(data));
        }
        this._relayFrom(fromId, data);
        this.dirty = true;
    }
    _onKickPlayer(fromId, data) {
        const r = new proto.Reader(data.slice(1));
        const targetId = r.readVarInt();
        if (r.error) return;
        const target = this.players.get(targetId);
        if (target) {
            const errBuf = proto.serializeError('You have been kicked');
            target.ws.send(errBuf);
            target.ws.close(1008, 'Kicked');
            this.removePlayer(targetId);
        }
    }
    _onBanPlayer(fromId, data) {
        const r = new proto.Reader(data.slice(1));
        const targetId = r.readVarInt();
        if (r.error) return;
        const target = this.players.get(targetId);
        if (target) {
            this.banned.add(target.name);
            const errBuf = proto.serializeError('You have been banned');
            target.ws.send(errBuf);
            target.ws.close(1008, 'Banned');
            this.removePlayer(targetId);
        }
    }
    _decayLocks() {
        const now = Date.now();
        for (const [uuid, lock] of this.locks) {
            if (now >= lock.expiresAt) {
                this.locks.delete(uuid);
            }
        }
    }
    _syncLevelTo(ws) {
        if (!this.compressedLevelData || this.compressedLevelData.length === 0) {
            ws.send(proto.serializeSyncLevelStart(1, 0, this.settings));
            ws.send(proto.serializeSyncLevelChunk(0, Buffer.alloc(0), []));
            const locks = this._getLocksArray();
            ws.send(proto.serializeSyncLevelEnd(locks));
            return;
        }
        const chunks = [];
        let byteOffset = 0;
        let uuidOffset = 0;
        while (byteOffset < this.compressedLevelData.length || uuidOffset < this.uuids.length) {
            const chunk = { data: Buffer.alloc(0), uuids: [] };
            const bytesToTake = Math.min(MAX_CHUNK_BYTES, this.compressedLevelData.length - byteOffset);
            if (bytesToTake > 0) {
                chunk.data = this.compressedLevelData.slice(byteOffset, byteOffset + bytesToTake);
                byteOffset += bytesToTake;
            }
            const uuidsToTake = Math.min(MAX_UUIDS_PER_CHUNK, this.uuids.length - uuidOffset);
            if (uuidsToTake > 0) {
                chunk.uuids = this.uuids.slice(uuidOffset, uuidOffset + uuidsToTake);
                uuidOffset += uuidsToTake;
            }
            chunks.push(chunk);
        }
        if (chunks.length === 0) {
            chunks.push({ data: Buffer.alloc(0), uuids: [] });
        }
        const totalChunks = chunks.length;
        const totalObjects = this.uuids.length;
        ws.send(proto.serializeSyncLevelStart(totalChunks, totalObjects, this.settings));
        for (let i = 0; i < totalChunks; i++) {
            ws.send(proto.serializeSyncLevelChunk(i, chunks[i].data, chunks[i].uuids));
        }
        const locks = this._getLocksArray();
        ws.send(proto.serializeSyncLevelEnd(locks));
    }
    _getLocksArray() {
        const locks = [];
        const now = Date.now();
        for (const [uuid, lock] of this.locks) {
            const remaining = Math.max(0, (lock.expiresAt - now) / 1000);
            if (remaining > 0) {
                locks.push({ uuid, playerId: lock.playerId, timeLeft: remaining });
            }
        }
        return locks;
    }
    requestSnapshot() {
        if (this.snapshotPending) return;
        if (this.players.size === 0) return;
        let target = null;
        for (const [, p] of this.players) {
            if (p.ws && p.ws.readyState === 1) {
                target = p;
                break;
            }
        }
        if (!target) return;
        this.snapshotPending = true;
        this.snapshotState = {
            fromPlayerId: target.id,
            totalChunks: 0,
            chunks: [],
            uuidChunks: [],
            settings: null,
            active: false,
        };
        target.ws.send(proto.serializeRequestSnapshot());
        console.log(`  [${this.code}] Requested snapshot from ${target.name}`);
    }
    _handleSnapshotResponse(playerId, opcode, data) {
        if (opcode === proto.Opcode.SyncLevelStart) {
            const r = new proto.Reader(data.slice(1));
            const msg = proto.deserializeSyncLevelStart(r);
            if (r.error) return true;
            this.snapshotState = {
                fromPlayerId: playerId,
                active: true,
                totalChunks: msg.totalChunks,
                settings: msg.settings,
                chunks: new Array(msg.totalChunks).fill(null),
                uuidChunks: new Array(msg.totalChunks).fill(null),
            };
            this.snapshotPending = true;
            return true;
        }

        const snap = this.snapshotState;
        if (!snap || !snap.active || playerId !== snap.fromPlayerId) return false;
        if (opcode === proto.Opcode.SyncLevelChunk && snap.active) {
            const r = new proto.Reader(data.slice(1));
            const msg = proto.deserializeSyncLevelChunk(r);
            if (r.error) return true;
            if (msg.chunkIndex < snap.totalChunks) {
                snap.chunks[msg.chunkIndex] = msg.data;
                snap.uuidChunks[msg.chunkIndex] = msg.uuids;
            }
            return true;
        }
        if (opcode === proto.Opcode.SyncLevelEnd && snap.active) {
            const r = new proto.Reader(data.slice(1));
            const msg = proto.deserializeSyncLevelEnd(r);
            const allData = [];
            const allUuids = [];
            for (let i = 0; i < snap.totalChunks; i++) {
                if (snap.chunks[i]) allData.push(snap.chunks[i]);
                if (snap.uuidChunks[i]) allUuids.push(...snap.uuidChunks[i]);
            }
            this.compressedLevelData = Buffer.concat(allData);
            this.uuids = allUuids;
            if (snap.settings) {
                this.settings = snap.settings;
                if (snap.settings.levelName) {
                    this.levelName = snap.settings.levelName;
                }
            }
            this.locks.clear();
            const now = Date.now();
            for (const lock of msg.locks) {
                this.locks.set(lock.uuid, {
                    playerId: lock.playerId,
                    expiresAt: now + (lock.timeLeft * 1000),
                });
            }
            this.dirty = false;
            this.snapshotPending = false;
            this.snapshotState = null;
            console.log(`  [${this.code}] Snapshot received (${allUuids.length} objects, ${this.compressedLevelData.length} bytes)`);
            if (this.onSnapshotSaved) this.onSnapshotSaved(this);
            return true;
        }
        return false;
    }
    _relayFrom(fromPlayerId, data) {
        const wrapped = proto.serializeRelay(fromPlayerId, data);
        this._broadcastExcept(wrapped, fromPlayerId);
    }
    _broadcastExcept(buf, excludeId) {
        for (const [id, player] of this.players) {
            if (id === excludeId) continue;
            if (player.ws && player.ws.readyState === 1) {
                player.ws.send(buf);
            }
        }
    }
    _broadcastAll(buf) {
        for (const [, player] of this.players) {
            if (player.ws && player.ws.readyState === 1) {
                player.ws.send(buf);
            }
        }
    }
}
module.exports = { Room, generateRoomCode, generateUUID };
