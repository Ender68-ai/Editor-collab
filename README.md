# Collaborative Editor Mod for Geometry Dash

A revolutionary real-time collaborative level editing system for Geometry Dash 2.2081, built with Geode 5.3.0 compatibility.

## 🎮 Features

### Core Collaboration
- **Real-time Object Synchronization** - Every object edit, move, or deletion instantly broadcasts to all connected players
- **Color-coded Player Ownership** - Each player gets a unique color (Red, Green, Blue, Yellow, Magenta, Cyan, Orange, Purple)
- **Live Player List** - Shows all connected players with their colors, names, and online status
- **In-editor Chat System** - Real-time messaging with 100-message history buffer
- **Cursor Indicators** - Visual representation of every player's mouse position in the editor
- **Session Management** - Host sessions, join existing ones, save/load collaborative work

### Technical Excellence
- **Thread-safe Networking** - Network operations separated from main game thread using std::mutex and std::queue
- **Bandwidth Optimization** - Configurable update frequency (5-60 Hz) for different connection speeds
- **Performance Modes** - Low-lag mode for high-latency connections
- **Object Ownership Tracking** - NodeID-based system with visual highlighting
- **Full-level Resync** - New players receive complete level state upon joining

### Professional Features
- **Geode 5.3.0 Compatible** - Proper dependencies and API structure
- **WebSocket Relay Server** - Node.js server with automatic player management
- **Comprehensive Settings** - Update frequency, low-lag mode, auto-color assignment, chat history size
- **Resource Management** - Proper file bundling and API exposure

## 🚀 Installation

### Quick Start
1. **Download**: Get the latest `CollabEditor.geode` folder
2. **Install**: Drop it into your Geode mods directory
3. **Start Server**: Run `npm install && npm start` in the server folder
4. **Launch Geometry Dash** and enable the mod
5. **Host Session**: Click "Host Session" in the collaborative panel
6. **Share Address**: Give `localhost:8080` or your IP to friends

### Server Requirements
- **Node.js 14+** for WebSocket relay server
- **Port 8080** (configurable) for client connections
- **No additional dependencies** - pure Node.js implementation

## 🎯 Usage Guide

### Hosting a Session
1. Enter the Geometry Dash level editor
2. Open the Collaborative Editor panel (appears automatically)
3. Click "Host Session" to start a server
4. Share your server address with collaborators
5. Begin editing - all changes sync in real-time
1. Open the level editor
2. Click the "Collab" button to open the collaboration panel
3. Click "Host Session"
4. The server will start automatically and you'll be connected
5. Share your server address with other players

### Joining a Session

1. Open the level editor
2. Click the "Collab" button to open the collaboration panel
3. Click "Join Session"
4. Enter the server address (e.g., `ws://localhost:8080`)
5. Enter your player name
6. Click "Connect"

### Controls

- **Collab Panel**: Toggle with the Collab button in the editor
- **Chat**: Type messages in the chat input field
- **Object Ownership**: Objects you edit are automatically claimed by you
- **Cursor Tracking**: Your cursor position is visible to other players
- **Settings**: Adjust update frequency and enable low-lag mode

## Architecture

### Client-Side (C++)

- **CollabManager**: Central singleton managing networking and state
- **Hooks**: Intercepts editor actions and broadcasts them
- **UI Components**: Player list, chat window, cursor indicators
- **Thread Safety**: Network operations run in separate threads
- **Packet Processing**: Main thread processes queued packets safely

### Server-Side (Node.js)

- **WebSocket Relay**: Simple relay server that broadcasts packets
- **Session Management**: Tracks connected players and sessions
- **No State Storage**: Server doesn't store level data, only relays packets
- **Web Interface**: Status page showing connected clients

### Packet Format

```json
{
  "type": "ACTION_TYPE",
  "playerID": "player_1234",
  "data": { ... }
}
```

Supported packet types:
- `PLAYER_JOIN` / `PLAYER_LEAVE`
- `CURSOR_UPDATE`
- `CHAT_MESSAGE`
- `ADD_OBJECT` / `REMOVE_OBJECT` / `MOVE_OBJECT`
- `CLAIM_OBJECT` / `TRANSFER_OWNERSHIP`
- `REQUEST_SYNC` / `FULL_SYNC`

## Configuration

### Settings

- **Update Frequency**: How often cursor updates are sent (default: 30Hz)
- **Low-Lag Mode**: Increases update frequency for better responsiveness
- **Show Cursors**: Toggle cursor visibility
- **Show Ownership**: Toggle object ownership coloring

### Performance

For large levels, consider:
- Reducing update frequency
- Enabling low-lag mode only when needed
- Using object filters to hide some players' edits

## Development

### Building

```bash
# Clone the repository
git clone <repository-url>
cd collab-editor

# Build using Geode's build system
geode build
```

### File Structure

```
src/
├── main.cpp              # Mod entry point and GameObject modifications
├── CollabManager.cpp     # Core networking and state management
├── Hooks.cpp             # Editor hooks for object actions
├── Resync.cpp            # Level synchronization logic
├── UI/                   # User interface components
│   ├── CollabPanel.cpp   # Main collaboration panel
│   ├── PlayerList.cpp    # Player list display
│   ├── ChatWindow.cpp    # Chat interface
│   └── CursorIndicator.cpp # Cursor tracking
├── Networking/           # WebSocket client implementation
│   └── WebSocketClient.cpp
├── Utils/                # Utility functions
│   └── ColorUtils.cpp    # Color management
└── PacketHandler.cpp     # Packet processing logic

server/
├── server.js             # Node.js WebSocket server
└── package.json          # Server dependencies
```

### Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly with multiple clients
5. Submit a pull request

## Troubleshooting

### Common Issues

- **Connection Failed**: Ensure the server is running and accessible
- **Objects Not Syncing**: Check that both clients are connected to the same server
- **High Latency**: Try enabling low-lag mode or reducing update frequency
- **Compilation Errors**: Ensure all dependencies are installed correctly

### Debug Mode

Enable debug logging in the mod settings to see detailed network activity.

## License

MIT License - see LICENSE file for details.

## Credits

- Geode SDK team for the modding framework
- BetterEdit team for undo/redo integration
- WebSocket++ library for networking
- Geometry Dash community for feedback and testing
