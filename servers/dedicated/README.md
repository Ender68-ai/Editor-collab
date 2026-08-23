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
4. Follow the terminal prompts to select the level to host and configure the server (Port, Max Players, Password, Autosave interval, and Default View-Only state).

## Playing over the Internet

To play with people outside your local network, you need a public address.

### Port Forwarding
1. Forward TCP port 7575 to your computer's local IP address in your router settings.
2. Players connect using your public IP: `ws://[your-public-ip]:7575`

### Tunnels
If you cannot port forward, use a tunnel service. I personally recommend ngrok or localtunnel, as they have both been tested and confirmed to work well.

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

*Note: To actually save the multiplayer level to your device, open the Editor Pause Menu and click **Save**. The dedicated server will always save and store levels independently in the levels folder and will never overwrite your local game level files.*

## Admin Commands

Use these commands in the server terminal:

* `/rooms` - List active rooms and player counts
* `/message <text>` - Broadcast a notification message to all connected players
* `/save` - Force a manual save to `[LevelName]_save.gmd`
* `/export` - Export the current level to `[LevelName]_export.gmd` in the `levels/` folder
* `/kick <playerId or Name>` - Kick a player from the server
* `/ban <playerId or Name>` - Ban a player from the server
* `/viewonly <playerId or Name> <on|off>` - Force a player into or out of view-only mode
* `/stop` - Shut down the server

## Saving & Auto-Saving

There are three ways levels are saved on the dedicated server (all saves go to the `levels/` folder):

1. **Auto-Saving:** When starting the server, you can specify an autosave interval in minutes. The server will automatically write a `[LevelName]_autosave.gmd` file in the background.
2. **Terminal `/save`:** Typing `/save` in the terminal forces an immediate manual save to `[LevelName]_save.gmd`. 
3. **In-Game "Save":** When a player clicks "Save" or "Save and Exit" inside Geometry Dash, it triggers the same manual save, creating or updating `[LevelName]_save.gmd`.

You can directly import any of these `.gmd` files into Geometry Dash at any time, or use the `/export` command in the terminal to generate a clean `[LevelName]_export.gmd`.