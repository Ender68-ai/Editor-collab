# Multiplayer Edit

<img src="logo.png" width="150" alt="Multiplayer Edit logo" />

Real-time collaborative level editing for Geometry Dash! Host a session, invite your friends with a room code, and build levels together in real-time.

## [Join the Official Discord Server!](https://discord.gg/mdsuxYu2YP)

## Features

- **Real-Time Collaborative Editing:** Host a session, share the room code, and build together.
- **Instant Synchronization:** Placement, deletion, rotation, scaling, and movement are updated across all connected clients.
- **Isolated Undo/Redo Stacks:** Action history is tracked per player, meaning your undo/redo actions won't overwrite other players' actions.
- **Player Cursors & Badges:** Track player movements live in the editor. Badges display next to player cursors showing the specific object they currently have selected.
- **Playtesting icon Sync:** Watch players playtest in the editor.
- **Smart Object Locking:** Automatically locks selected objects to prevent multiple players from editing the same objects, ensuring no race conditions or crashes.
- **Session HUD & Player List:** Keep track of room details with a player list and status overlay showing the active session code.
- **Join/Leave Notifications:** In-game notifications alert you when players enter or leave the room.

## How to Use

### Installation

1. Make sure you have [Geode](https://geode-sdk.org/) installed.
2. Go to the [Releases page](https://github.com/xXoanon/MultiplayerEdit/releases) and download the latest `.geode` file.
3. Place the `.geode` file in your Geometry Dash `geode/mods/` folder.
4. Restart Geometry Dash.

### Using the Mod
1. Open any level in the editor.
2. Click the **Multiplayer** button (in the pause menu if you're hosting, or on the "my levels" page if you're joining).
3. Host a room, find a public lobby, or type in your friend's code to join theirs.

## Building from source

You'll need the [Geode SDK and CLI](https://docs.geode-sdk.org/).

```sh
git clone https://github.com/xXoanon/MultiplayerEdit.git
cd MultiplayerEdit
geode build
```

To build for specific platforms (like Android, or Windows on Linux):
```sh
geode build --platform win
geode build --platform android64
```

## Signaling Server

The mod uses WebRTC for P2P connections between players. To actually find each other though, it needs a signaling server. By default, it connects to a free public one I made (`https://dewy-flea-9364.d050.deno.net`), but you can host your own. 

See the [server/README.md](server/README.md) file for setup and hosting instructions. You can update the **Signaling Server URL** setting in the mod settings in-game to connect to your custom server.

> **Note**: It is not recommended to use the old NodeJS WebSocket relay server (version 0.3.0 and older). Those older versions are much buggier and less stable compared to the newer P2P WebRTC releases.

## Support Development

If you enjoy this mod or any of my other projects, consider supporting future development on [Patreon](https://www.patreon.com/c/d050/membership)!
