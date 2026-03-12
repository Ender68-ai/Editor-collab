# Collaborative Editor Mod for Geometry Dash

A real-time collaborative level editing system for Geometry Dash 2.2081.

## Features

### 🎮 Core Collaboration
- **Real-time object synchronization** - All object edits broadcast instantly to connected clients
- **Color-coded player ownership** - Each player gets unique color for their objects
- **Live player list** - Shows all connected players with their colors and status
- **In-editor chat system** - Real-time chat with message history
- **Cursor indicators** - Visual cursors showing all players' mouse positions
- **Session management** - Host, join, save, and load collaborative sessions

### 🛠️ Technical Features
- **Thread-safe networking** - Network operations separated from main thread
- **Bandwidth optimization** - Throttled cursor updates and configurable sync frequency
- **Performance modes** - Low-lag mode for high-latency connections
- **Object ownership tracking** - NodeID-based system with visual highlighting
- **Full-level resync** - New players get complete level state on join

### 🚀 Installation

1. **Install Mod**: Copy `CollabEditor.geode` folder to your Geode mods directory
2. **Start Server**: Run `npm install && npm start` in the server folder
3. **Launch Game**: Start Geometry Dash and enable the mod
4. **Host Session**: Use in-game UI to host a collaborative session
5. **Join Session**: Other players connect using `localhost:8080` or your IP

### 📁 Files Included

```
CollabEditor.geode/
├── mod.json              # Mod configuration and metadata
├── CollabEditor.dll      # Main mod binary
└── server/               # WebSocket relay server
    ├── server.js          # Node.js server implementation
    └── package.json       # Server dependencies
```

### 🎯 Usage

#### Hosting a Session:
1. Open Geometry Dash and enter the level editor
2. Click "Host Session" in the collaborative panel
3. Share the server address (localhost:8080) with other players
4. Start editing - all changes are synchronized in real-time

#### Joining a Session:
1. Get the server address from the host
2. Open Geometry Dash and enter the level editor  
3. Click "Join Session" and enter the server address
4. Begin collaborative editing with full synchronization

### 🔧 Configuration

The mod includes performance settings:
- **Update Frequency**: Controls how often cursor/object updates are sent
- **Low-Lag Mode**: Reduces update frequency for better performance on slow connections
- **Color Assignment**: Automatic unique color assignment for each player
- **Chat History**: Keeps last 100 chat messages in memory

### 🌐 Network Protocol

The mod uses a simple JSON-based protocol:
- `PLAYER_JOIN` - New player connected
- `PLAYER_LEAVE` - Player disconnected  
- `OBJECT_ACTION` - Object add/remove/move operations
- `CURSOR_UPDATE` - Player cursor position updates
- `CHAT` - Chat messages with player info
- `FULL_SYNC` - Complete level state for new players

### 🤝 Compatibility

- **Geometry Dash 2.2081** - Targeted version support
- **Geode Loader v5.3.0** - Full compatibility with latest Geode
- **BetterEdit Integration** - Optional enhanced features when BetterEdit is detected
- **Windows Platform** - Optimized for Windows 10/11

---

**Ready for collaborative level editing! 🎉**

Created with ❤️ for the Geometry Dash community
