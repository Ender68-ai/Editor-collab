# Signaling Server

WebRTC signaling server for MultiplayerEdit. Facilitates SDP offer/answer and ICE candidate exchange so peers can establish direct P2P connections.

## What This Does

- **Only signaling:** This server helps players find each other and exchange connection info. Once a WebRTC connection is established, all game data flows directly between players. The server never sees or relays game data.
- Room-based matchmaking with 6-character room codes
- Supports up to 8 players per room
- Rooms auto-expire after 2 hours

## Deploy to Deno Deploy

1. Go to [dash.deno.com](https://dash.deno.com) and sign in
2. Create a new project
3. Link to this repo and set the entrypoint to `server/signaling/worker.js`
4. Deploy — that's it

Your server URL will be something like `https://your-project.deno.dev`.

## Run Locally

```bash
deno run --allow-net worker.js
```

Server starts on `http://localhost:8000`.

## API Endpoints

| Method | Endpoint | Description |
|--------|----------|-------------|
| POST | `/rooms` | Create room. Body: `{playerName}` |
| GET | `/rooms/:code` | Get room info |
| POST | `/rooms/:code/join` | Join room. Body: `{playerName}` |
| POST | `/rooms/:code/offer` | Send SDP offer. Body: `{sdp, targetPlayerId}` |
| GET | `/rooms/:code/offer?playerId=N` | Poll for SDP offer |
| POST | `/rooms/:code/answer` | Send SDP answer. Body: `{sdp, playerId}` |
| GET | `/rooms/:code/answer?playerId=N` | Poll for SDP answer |
| POST | `/rooms/:code/ice` | Send ICE candidates. Body: `{playerId, candidates[], isHost}` |
| GET | `/rooms/:code/ice?playerId=N&isHost=bool` | Poll for ICE candidates |
| DELETE | `/rooms/:code` | Close room |
| GET | `/health` | Health check |
