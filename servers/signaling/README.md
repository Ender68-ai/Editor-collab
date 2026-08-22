# Multiplayer Edit Signaling Server

This is the Deno Deploy signaling server that handles matchmaking and WebRTC SDP exchange for the Multiplayer Edit mod.

Because the mod uses WebRTC Data Channels, players connect directly to each other peer to peer. This server is **only** used for the initial handshake to exchange IP addresses and connection metadata. Once a player joins a room, all game data flows directly between players.

## Requirements

- A [Deno Deploy](https://deno.com/deploy) account

## Setup and Hosting

1. Go to [Deno Deploy](https://dash.deno.com) and click **New Playground**.
2. Copy the contents of `signaling/worker.js` and paste it into the editor, replacing the contents of `main.ts`.
3. Click the **"Databases"** tab on your new project and connect a new KV database to it.
4. Click **Deploy**.
5. Copy the URL of your new playground.

## Using Your Custom Server

In Geometry Dash, go to the Multiplayer Edit mod settings and change the **Signaling Server URL** to the URL you copied above. Make sure it uses `https://`.

## How It Works

1. **Host** creates a room → POSTs to `/rooms` and receives a 6-character room code.
2. **Guests** join using the room code → POSTs to `/rooms/:code/join` and gets the Host's metadata.
3. **Guest** generates a WebRTC Offer and POSTs it to the signaling server.
4. **Host** polls for Offers, retrieves the Guest's Offer, generates an Answer, and POSTs it back.
5. **Guest** polls for Answers and retrieves the Host's Answer.
6. The direct P2P connection is established! The signaling server is no longer used for this session.

## Features & Limits

- **Auto-Expiration:** Rooms automatically expire and are cleaned up after 5 minutes of inactivity.

## API Endpoints

| Method | Endpoint | Description |
|--------|----------|-------------|
| POST | `/rooms` | Create room. Body: `{hostName, roomName, description, playerLimit, isPrivate, hasPassword, password, version}` |
| GET | `/rooms/:code` | Get room info |
| POST | `/rooms/:code/join` | Join room. Body: `{playerName, password}` |
| POST | `/rooms/:code/leave` | Leave room. Body: `{playerId}` |
| POST | `/rooms/:code/signal` | Send WebRTC message (Offer, Answer, or ICE). Body: `{type, ...msg}` |
| GET | `/rooms/:code/signal?role=<host\|client>&playerId=N` | Poll for pending WebRTC messages using long-polling |
| POST | `/rooms/:code/ban` | Ban a player from a room. Body: `{playerName}` |
| DELETE | `/rooms/:code` | Close room (Host only) |
| GET | `/health` | Health check |
