# Multiplayer Edit Dedicated Server

This is the Node.js dedicated server for the Multiplayer Edit mod.

It reads levels from your local `CCLocalLevels.dat` save file, hosts them in memory, and syncs edits across all connected players. Unlike standard P2P hosting, Geometry Dash does not need to be open to keep the server running.

## Requirements

- [Node.js](https://nodejs.org/)

## Setup and Hosting

1. Install dependencies:
   ```bash
   npm install
   ```
2. Start the server:
   ```bash
   npm start
   ```
3. The server will ask you where you want to load levels from:
   - **My Geometry Dash Saves:** Automatically locates your `CCLocalLevels.dat` file and lists your game's levels.
   - **Local .gmd files:** Reads `.gmd` files from the `levels/` folder next to the script.
   - **Custom file path:** Paste a direct path to any `.gmd` file on your system.
4. Follow the terminal prompts to select the level(s) you want to host.
5. The server starts on port 7575 by default.

## Playing over the Internet

To play with people outside your local network, you need a public address.

### Port Forwarding
1. Forward TCP port 7575 to your computer's local IP address in your router settings.
2. Players connect using your public IP: `ws://[your-public-ip]:7575`

### Tunnels
If you cannot port forward, use a tunnel service.

**ngrok:**
```bash
ngrok http 7575
```
This gives you a URL like `https://1234.ngrok-free.app`. Use it to connect in-game.

**localtunnel:**
```bash
npx localtunnel --port 7575
```
This gives you a URL like `https://my-server.loca.lt`. Use it to connect in-game.

## Connecting from Geometry Dash

1. Click the **Servers** button in the Multiplayer menu.
2. Click **Add Server** and enter the server's URL.
3. Click **Join**.

*Note: To actually save the multiplayer level to your device, open the Editor Pause Menu and click **Save**.*

## Admin Commands

Use these commands in the server terminal:

* `/rooms` - List active rooms and player counts
* `/save` - Force a save for all rooms
* `/export <roomCode>` - Export a room's state to a `.gmd` file in the `saves/` folder
* `/kick <roomCode> <playerId>` - Kick a player
* `/ban <roomCode> <playerId>` - Ban a player
* `/stop` - Shut down the server

## Auto-Saving

The server requests a level snapshot from clients every 5 minutes. This is saved to `saves/[LevelName]_autosave.gmd`. 

You can directly import this backup into Geometry Dash at any time, or use the `/export` command to generate a manual snapshot.
