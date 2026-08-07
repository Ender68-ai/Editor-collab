# Multiplayer Edit

Collaborate with friends in real-time level editing! Host a session and build together.

## Features

- **Host & Join Sessions** — Create a room with a 6-character code and invite friends to join.
- **Live Sync** — Object placement, deletion, movement, rotation, and scaling sync instantly across all players.
- **Isolated Undo/Redo** — Action stacks are kept local, ensuring undoing a placement doesn't interfere with other players.
- **Player Cursors & Equipped Badges** — View live player cursors with tool badges showing the object they currently have selected.
- **Real-time Playtest Icons** — Watch players playtest in real time with their actual icons.
- **Edit Locking** — Smart selection locks prevent multiple players from editing the same objects simultaneously, preventing conflicts or crashes.
- **In-Editor HUD** — Displays connection state, room code, and a player list.
- **Notifications** — Alerts when players join or leave the session.

## How to Use

1. Open any level in the editor.
2. Click the **Multiplayer** button in the editor pause menu if hosting, or on the "My Levels" page if joining.
3. **Host** a new session to generate a room code, or **Join** using a code shared by your friend.
4. Build, edit, and playtest together in real-time!

## Server Configuration

This mod establishes direct Peer-to-Peer (P2P) connections between players using WebRTC. However, it requires a lightweight signaling server to exchange initial connection information to matchmake players. A public default signaling server is configured by default, but you can host your own.

Setup instructions for hosting your own Deno signaling server are located in the `server/` directory of the source repository. You can update the **Signaling Server URL** setting in the mod settings in-game to connect to your custom server.