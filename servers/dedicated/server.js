#!/usr/bin/env node
const readline = require('readline');
const saveReader = require('./src/save-reader');
const { RoomManager } = require('./src/room-manager');
const { WSServer } = require('./src/ws-server');

async function main() {
    const fs = require('fs');
    const path = require('path');
    const prompts = require('prompts');
    
    console.log('\x1b[36m\x1b[1m MultiplayerEdit Dedicated Server \x1b[0m\n');
    
    const modeResponse = await prompts({
        type: 'select',
        name: 'mode',
        message: 'Where do you want to load levels from?',
        choices: [
            { title: 'My Geometry Dash Saves (CCLocalLevels.dat)', value: 'gd' },
            { title: 'Local .gmd files (from the levels folder)', value: 'gmd' },
            { title: 'Enter a custom file path...', value: 'custom' }
        ]
    });

    if (!modeResponse.mode) {
        process.exit(0);
    }

    const levels = [];

    if (modeResponse.mode === 'gd') {
        const savePath = saveReader.findSaveFile();
        if (!savePath || !fs.existsSync(savePath)) {
            console.error('\x1b[31m[ERROR]\x1b[0m Could not locate CCLocalLevels.dat. Verify Geometry Dash is installed.');
            process.exit(1);
        }
        console.log(`\n\x1b[36m[INFO]\x1b[0m Reading CCLocalLevels.dat: ${savePath}...`);
        try {
            const xml = saveReader.decryptSaveFile(savePath);
            levels.push(...saveReader.parseLevels(xml));
        } catch (e) {
            console.error('\x1b[31m[ERROR]\x1b[0m Failed to parse CCLocalLevels.dat:', e.message);
            process.exit(1);
        }
    } else if (modeResponse.mode === 'gmd') {
        const levelsDir = path.join(process.cwd(), 'levels');
        if (!fs.existsSync(levelsDir)) {
            fs.mkdirSync(levelsDir, { recursive: true });
        }
        const files = fs.readdirSync(levelsDir).filter(f => f.endsWith('.gmd'));
        if (files.length === 0) {
            console.error('\n\x1b[31m[ERROR]\x1b[0m No .gmd files found in levels/ directory.');
            process.exit(1);
        }
        console.log('');
        for (const file of files) {
            console.log(`\x1b[36m[INFO]\x1b[0m Loading ${file}...`);
            try {
                const xml = fs.readFileSync(path.join(levelsDir, file), 'utf8');
                const parsed = saveReader.parseGmd(xml);
                parsed.forEach(l => l.filename = file);
                levels.push(...parsed);
            } catch (e) {
                console.error(`\x1b[31m[ERROR]\x1b[0m Failed to read ${file}:`, e.message);
            }
        }
    } else if (modeResponse.mode === 'custom') {
        const rl = readline.createInterface({ input: process.stdin, output: process.stdout });
        const manualPath = await new Promise((resolve) => rl.question('\x1b[33mEnter full path to a .gmd file:\x1b[0m ', resolve));
        rl.close();
        if (manualPath && fs.existsSync(manualPath)) {
            if (!manualPath.endsWith('.gmd')) {
                console.error('\n\x1b[31m[ERROR]\x1b[0m Only .gmd files are supported.');
                process.exit(1);
            }
            try {
                const xml = fs.readFileSync(manualPath, 'utf8');
                const parsed = saveReader.parseGmd(xml);
                parsed.forEach(l => l.filename = path.basename(manualPath));
                levels.push(...parsed);
            } catch (e) {
                console.error('\x1b[31m[ERROR]\x1b[0m Failed to read file:', e.message);
                process.exit(1);
            }
        } else {
            console.error('\n\x1b[31m[ERROR]\x1b[0m Invalid path or file does not exist.');
            process.exit(1);
        }
    }

    if (levels.length === 0) {
        console.error('\n\x1b[31m[ERROR]\x1b[0m No levels found to host. Exiting...');
        process.exit(1);
    }
    const response = await prompts([
        {
            type: 'autocomplete',
            name: 'selectedLevel',
            message: 'Search and select a level to host',
            choices: levels.map((l, i) => {
                const prefix = l.filename ? `${l.filename} - ` : '';
                return { title: `[${i + 1}] ${prefix}${l.name} (${l.objectCount} objects)`, value: l };
            }),
        },
        {
            type: 'number',
            name: 'port',
            message: 'Port',
            initial: 7575
        },
        {
            type: 'number',
            name: 'maxPlayers',
            message: 'Max players per room',
            initial: 100
        },
        {
            type: 'text',
            name: 'password',
            message: 'Room password (leave blank for none)',
            initial: ''
        },
        {
            type: 'number',
            name: 'autosaveInterval',
            message: 'Autosave interval in minutes (0 to disable)',
            initial: 5
        },
        {
            type: 'toggle',
            name: 'defaultViewOnly',
            message: 'Default new players to view-only mode?',
            initial: false,
            active: 'yes',
            inactive: 'no'
        }
    ]);

    if (!response.selectedLevel) {
        console.error('No level selected. Exiting...');
        process.exit(0);
    }

    const selectedLevel = response.selectedLevel;
    const port = (typeof response.port === 'number' && isFinite(response.port) && response.port > 0) ? response.port : 7575;
    const maxPlayers = (typeof response.maxPlayers === 'number' && isFinite(response.maxPlayers) && response.maxPlayers > 0) ? response.maxPlayers : 100;
    const roomPassword = response.password || "";
    const autosaveInterval = (typeof response.autosaveInterval === 'number' && isFinite(response.autosaveInterval) && response.autosaveInterval >= 0) ? response.autosaveInterval : 5;
    const defaultViewOnly = !!response.defaultViewOnly;
    
    const roomManager = new RoomManager();
    const wsServer = new WSServer(roomManager);
    console.log('\nStarting server...');
    roomManager.start(port, false); 
    
    console.log(`Decoding level data for "${selectedLevel.name}"...`);
    const decoded = saveReader.decodeLevelString(selectedLevel.levelString);
    const settings = {
        saveString: decoded.settings,
        audioTrack: selectedLevel.audioTrack,
        songID: selectedLevel.songID,
        levelLength: 0,
        levelName: selectedLevel.name,
        password: roomPassword,
        defaultViewOnly: defaultViewOnly
    };
    const objectCount = decoded.objects.split(';').filter(Boolean).length;
    const crypto = require('crypto');
    const uuids = Array.from({ length: objectCount }, () => crypto.randomUUID());
    const room = roomManager.createRoom(selectedLevel.name, { compressedBytes: Buffer.from(decoded.objects, 'utf8'), uuids: uuids }, settings);
    room.maxPlayers = maxPlayers;
    room.password = roomPassword;
    console.log(`\x1b[32m\x1b[1m✓ Room created: "${room.levelName}"\x1b[0m`);
    
    if (autosaveInterval > 0) {
        setInterval(() => {
            roomManager._performAutoSave();
        }, autosaveInterval * 60000);
        console.log(`\x1b[32m✓ Autosave enabled (every ${autosaveInterval} minutes)\x1b[0m`);
    }
    
    await wsServer.start(port);
    console.log(`\n\x1b[36m\x1b[1mServer running on port ${port}!\x1b[0m`);
    console.log(`\x1b[33mPlayers can connect by entering 'ws://<your-ip>:${port}' in the game.\x1b[0m`);
    console.log('\n\x1b[1mAdmin commands:\x1b[0m /kick <id>, /ban <id>, /viewonly <id> <on|off>, /message <text>, /save, /export, /stop\n');
    
    const rlAdmin = readline.createInterface({
        input: process.stdin,
        output: process.stdout
    });
    
    rlAdmin.on('line', (line) => {
        const parts = line.trim().split(' ');
        const cmd = parts[0].toLowerCase();
        const args = parts.slice(1);
        
        const room = roomManager.rooms.values().next().value;
        if (!room) {
            console.log('No active room.');
            return;
        }

        const resolvePlayer = (arg) => {
            const id = parseInt(arg);
            if (!isNaN(id) && room.players.has(id)) return id;
            for (const [pid, p] of room.players) {
                if (p.name && p.name.toLowerCase() === arg.toLowerCase()) return pid;
            }
            return null;
        };

        if (cmd === '/stop') {
            console.log('\x1b[31m\x1b[1m[ADMIN]\x1b[0m Stopping server...');
            wsServer.stop();
            roomManager.stop();
            process.exit(0);
        } else if (cmd === '/save') {
            console.log('\x1b[36m\x1b[1m[ADMIN]\x1b[0m Saving level to disk...');
            const levelsDir = require('path').join(process.cwd(), 'levels');
            const outFile = saveReader.exportToGmd(room, levelsDir, '_save');
            // Log will be handled by room-manager.js _saveRoomToDisk
        } else if (cmd === '/message') {
            if (args.length === 0) return console.log('\x1b[33mUsage:\x1b[0m /message <text>');
            const text = args.join(' ');
            const proto = require('./src/protocol');
            const w = new proto.Writer();
            w.writeOpcode(proto.Opcode.ServerMessage);
            w.writeString(text);
            room._relayFrom(0, w.finish());
            console.log(`\x1b[36m\x1b[1m[ADMIN]\x1b[0m Broadcasted message: ${text}`);
        } else if (cmd === '/kick') {
            if (args.length !== 1) return console.log('\x1b[33mUsage:\x1b[0m /kick <playerId or Name>');
            const pid = resolvePlayer(args[0]);
            if (pid !== null) {
                room._onKickPlayer(0, Buffer.concat([Buffer.from([0]), Buffer.from([pid])])); 
                console.log(`\x1b[31m\x1b[1m[ADMIN]\x1b[0m Kicked player ${args[0]}`);
            } else console.log('\x1b[31m\x1b[1m[ADMIN]\x1b[0m Player not found');
        } else if (cmd === '/ban') {
            if (args.length !== 1) return console.log('\x1b[33mUsage:\x1b[0m /ban <playerId or Name>');
            const pid = resolvePlayer(args[0]);
            if (pid !== null) {
                room._onBanPlayer(0, Buffer.concat([Buffer.from([0]), Buffer.from([pid])]));
                console.log(`\x1b[31m\x1b[1m[ADMIN]\x1b[0m Banned player ${args[0]}`);
            } else console.log('\x1b[31m\x1b[1m[ADMIN]\x1b[0m Player not found');
        } else if (cmd === '/viewonly') {
            if (args.length !== 2 || (args[1] !== 'on' && args[1] !== 'off')) return console.log('\x1b[33mUsage:\x1b[0m /viewonly <playerId or Name> <on|off>');
            const pid = resolvePlayer(args[0]);
            if (pid !== null) {
                const isOn = args[1] === 'on';
                const proto = require('./src/protocol');
                const w = new proto.Writer();
                w.writeOpcode(proto.Opcode.SetViewOnly);
                w.writeU32(pid);
                w.writeBool(isOn);
                room._relayFrom(0, w.finish());
                console.log(`\x1b[36m\x1b[1m[ADMIN]\x1b[0m Set view-only to ${isOn} for player ${args[0]}`);
            } else console.log('\x1b[31m\x1b[1m[ADMIN]\x1b[0m Player not found');
        } else if (cmd === '/rooms') {
            console.log(`  \x1b[36m[INFO]\x1b[0m "${room.levelName}" - ${room.players.size}/${room.maxPlayers} players`);
        } else if (cmd === '/export') {
            const levelsDir = require('path').join(process.cwd(), 'levels');
            const outFile = saveReader.exportToGmd(room, levelsDir);
            console.log(`\x1b[35m[EXPORT]\x1b[0m Exported to ${outFile}`);
        } else {
            console.log('\x1b[31mUnknown command\x1b[0m');
        }
    });
}
main().catch(console.error);
