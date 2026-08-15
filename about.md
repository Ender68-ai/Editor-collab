# Multiplayer Edit

Collaborate with friends in real-time level editing! Host a session and build together.

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

1. Open any level in the editor.
2. Click the **Multiplayer** button in the editor pause menu if hosting, or on the "My Levels" page if joining.
3. Click **Host** to create a session and share the room code, or click **Join** and enter your friend's room code, or join through the public room browser.
4. Once connected, your changes will sync automatically!

## Server Configuration

This mod establishes direct Peer-to-Peer (P2P) connections between players using WebRTC. However, it requires a lightweight signaling server to exchange initial connection information to matchmake players. A public default signaling server is configured by default, but you can host your own.

Setup instructions for hosting your own Deno signaling server are located in the `server/` directory of the source repository. You can update the **Signaling Server URL** setting in the mod settings in-game to connect to your custom server.

## Support

If you enjoy this mod and want to support my work, you can do so on my [Patreon](https://www.patreon.com/cw/d050/membership)!