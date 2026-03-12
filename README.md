# Collaborative Editor Mod for Geometry Dash

A real-time collaborative level editor system for Geometry Dash using the Geode modding framework.

## Features

- **Real-time collaboration**: Multiple users can edit the same level simultaneously
- **Color-coded ownership**: Each player gets a unique color for their objects
- **Live player list**: See who's currently in the session
- **Cursor indicators**: Visual indicators showing other players' cursor positions
- **Chat system**: In-editor chat for communication
- **Collaborative undo/redo**: Integrates with BetterEdit's undo system
- **Session hosting**: Host sessions directly from the game
- **Object ownership tracking**: See who owns which objects
- **Performance controls**: Adjustable update frequency and low-lag mode
- **Session persistence**: Save and restore collaboration sessions

## Installation

### Prerequisites

- Geometry Dash version 2.2081
- Geode modding framework v5.3.0
- BetterEdit (recommended for full functionality)

### Server Setup

1. Install Node.js dependencies:
```bash
cd server
npm install
```

2. Start the server:
```bash
npm start
```

The server will run on `ws://localhost:8080` by default. You can specify a different port:
```bash
npm start 9090
```

### Mod Installation

1. Build the mod using Geode's build system
2. Place the compiled `.geode` file in your Geode mods folder
3. Launch Geometry Dash

## Usage

### Hosting a Session

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
