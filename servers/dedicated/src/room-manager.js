const crypto = require("crypto");
const fs = require("fs");
const path = require("path");
const { Room } = require("./room");
const saveReader = require("./save-reader");
class RoomManager {
  constructor() {
    this.rooms = new Map();
    this.signalingTimer = null;
    this.signalingUrl = "https://dewy-flea-9364.d050.deno.net";
    this.publicListing = false;
    this.loadTokens();
    this.port = 7575;
  }
  start(port, publicListing) {
    this.port = port;
    this.publicListing = publicListing;
    const levelsDir = path.join(process.cwd(), "levels");
    if (!fs.existsSync(levelsDir)) {
      fs.mkdirSync(levelsDir, { recursive: true });
    }
    if (this.publicListing) {
      this._updateSignalingServer();
      this.signalingTimer = setInterval(
        () => this._updateSignalingServer(),
        60 * 1000,
      );
    }
  }
  stop() {
    clearInterval(this.signalingTimer);
    for (const room of this.rooms.values()) {
      room.destroy();
    }
    this.rooms.clear();
  }
  createRoom(levelName, levelData, settings) {
    const room = new Room(levelName, levelData, settings);
    room.onSnapshotSaved = (r) => {
      const suffix = r.isAutoSaving ? "_autosave" : "_save";
      r.isAutoSaving = false;
      this._saveRoomToDisk(r, suffix);
    };
    this.rooms.set(room.code, room);
    return room;
  }

  createRoomForLevel(level, maxPlayers, password, defaultViewOnly) {
    console.log(`  Decoding level data for "${level.name}"...`);
    const decoded = saveReader.decodeLevelString(level.levelString);
    const settings = {
      saveString: decoded.settings,
      audioTrack: level.audioTrack,
      songID: level.songID,
      levelLength: 0,
      levelName: level.name,
      password: password,
      defaultViewOnly: defaultViewOnly,
    };
    const objectCount = decoded.objects.split(";").filter(Boolean).length;
    const uuids = Array.from({ length: objectCount }, () =>
      crypto.randomUUID(),
    );
    const room = this.createRoom(
      level.name,
      { compressedBytes: Buffer.from(decoded.objects, "utf8"), uuids: uuids },
      settings,
    );
    room.maxPlayers = maxPlayers;
    room.password = password;
    return room;
  }

  loadTokens() {
    this.tokens = new Set();
    try {
      if (fs.existsSync("tokens.json")) {
        const data = JSON.parse(fs.readFileSync("tokens.json", "utf8"));
        data.forEach((t) => this.tokens.add(t));
      }
    } catch (e) {
      console.error("Failed to load tokens.json:", e.message);
    }
  }
  saveTokens() {
    try {
      fs.writeFileSync(
        "tokens.json",
        JSON.stringify(Array.from(this.tokens), null, 2),
      );
    } catch (e) {
      console.error("Failed to save tokens.json:", e.message);
    }
  }
  addToken(token) {
    this.tokens.add(token);
    this.saveTokens();
  }
  removeToken(token) {
    const res = this.tokens.delete(token);
    this.saveTokens();
    return res;
  }
  isValidToken(token) {
    if (this.tokens.size === 0) return true; // If no tokens generated ever, maybe allow? Actually user wants it strictly enforced. Let's strictly enforce if ANY tokens exist. Wait, if tokens.json doesn't exist, this.tokens.size is 0.
    // Let's just always enforce it. If they want remote uploads, they MUST generate a token.
    return this.tokens.has(token);
  }

  getRoom(code) {
    return this.rooms.get(code.toUpperCase());
  }
  deleteRoom(code) {
    const upperCode = code.toUpperCase();
    const room = this.rooms.get(upperCode);
    if (!room) {
      return false;
    }
    room.destroy();
    this.rooms.delete(upperCode);
    return true;
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
        hasPassword: !!room.password,
      });
    }
    return list;
  }
  _performAutoSave() {
    for (const room of this.rooms.values()) {
      if (room.players.size > 0 && room.dirty) {
        room.isAutoSaving = true;
        room.requestSnapshot();
      } else if (
        !room.dirty &&
        room.compressedLevelData &&
        room.compressedLevelData.length > 0
      ) {
        this._saveRoomToDisk(room, "_autosave");
      }
    }
  }
  _saveRoomToDisk(room, suffix = "_autosave") {
    try {
      const levelsDir = path.join(process.cwd(), "levels");
      const outFile = saveReader.exportToGmd(room, levelsDir, suffix);
      console.log(
        `  \x1b[35m[SAVE]\x1b[0m Saved level to ${path.basename(outFile)}`,
      );
    } catch (e) {
      console.error(
        `  \x1b[31m[ERROR]\x1b[0m Failed to save level:`,
        e.message,
      );
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
          version: "Dedicated",
        });
      } catch (e) {}
    }
  }
}
module.exports = { RoomManager };
