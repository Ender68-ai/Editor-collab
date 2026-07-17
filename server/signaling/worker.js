


const kv = await Deno.openKv();
const ROOM_TTL = 2 * 60 * 60 * 1000; // 2 hours
const DEFAULT_MAX_PLAYERS = 8;  
const SERVER_MAX_PLAYERS = parseInt(Deno.env.get("MAX_PLAYERS") || "16");
const CHARS = "ABCDEFGHJKMNPQRSTUVWXYZ23456789"; // no 0/O/1/I/L

async function genCode() {
  let code;
  let exists = true;
  while (exists) {
    code = Array.from({ length: 6 }, () => CHARS[Math.floor(Math.random() * CHARS.length)]).join("");
    const res = await kv.get(["rooms", code]);
    exists = !!res.value;
  }
  return code;
}

function json(data, status = 200) {
  return new Response(JSON.stringify(data), {
    status, headers: {
      "Content-Type": "application/json",
      "Access-Control-Allow-Origin": "*",
      "Access-Control-Allow-Methods": "GET,POST,DELETE,OPTIONS",
      "Access-Control-Allow-Headers": "Content-Type",
    },
  });
}

Deno.serve(async (req) => {
  if (req.method === "OPTIONS") return json({ ok: true });

  const url = new URL(req.url);
  const parts = url.pathname.split("/").filter(Boolean); // ["rooms", code?, action?]


  if (parts[0] === "health" && req.method === "GET") {
    return json({ status: "ok" });
  }

  if (parts[0] !== "rooms") return json({ error: "not found" }, 404);

  if (parts.length === 1 && req.method === "POST") {  
    const { playerName, maxPlayers } = await req.json();  
    const code = await genCode();  
    const roomId = crypto.randomUUID();  
      
    const roomMaxPlayers = Math.min(maxPlayers || DEFAULT_MAX_PLAYERS, SERVER_MAX_PLAYERS);  
      
    await kv.set(["rooms", code], {  
      roomId, hostName: playerName, nextId: 1, created: Date.now(),  
      players: [{ id: 0, name: playerName }], maxPlayers: roomMaxPlayers  
    }, { expireIn: ROOM_TTL });  
      
    return json({ roomCode: code, roomId });  
}

  const code = parts[1]?.toUpperCase();
  const roomRes = await kv.get(["rooms", code]);
  const room = roomRes.value;
  
  if (!room) return json({ error: "room not found" }, 404);


  if (parts.length === 2 && req.method === "GET") {
    return json({ roomCode: code, hostName: room.hostName, playerCount: room.players.length, roomId: room.roomId });
  }


  if (parts.length === 2 && req.method === "DELETE") {
    await kv.delete(["rooms", code]);
    return json({ ok: true });
  }

  const action = parts[2];


  if (action === "join" && req.method === "POST") {  
    if (room.players.length >= (room.maxPlayers || DEFAULT_MAX_PLAYERS)) return json({ error: "room full" }, 400);  
    const { playerName } = await req.json();
    

    let success = false;
    let playerId = -1;
    let retries = 5;
    
    while (!success && retries > 0) {
      const currentRes = await kv.get(["rooms", code]);
      const currentRoom = currentRes.value;
      if (!currentRoom) return json({ error: "room deleted" }, 404);
      
      playerId = currentRoom.nextId++;
      currentRoom.players.push({ id: playerId, name: playerName });
      
      const commit = await kv.atomic()
        .check(currentRes)
        .set(["rooms", code], currentRoom, { expireIn: ROOM_TTL })
        .commit();
        
      success = commit.ok;
      retries--;
    }
    
    if (!success) return json({ error: "concurrent join failed" }, 500);
    
    return json({ playerId, hostName: room.hostName });
  }


  if (action === "offer") {
    if (req.method === "POST") {
      const { sdp, targetPlayerId } = await req.json();
      await kv.set(["offers", code, targetPlayerId], sdp, { expireIn: 60000 });
      return json({ ok: true });
    }
    if (req.method === "GET") {
      const pid = Number(url.searchParams.get("playerId"));
      const res = await kv.get(["offers", code, pid]);
      if (!res.value) return json({ sdp: null });
      await kv.delete(["offers", code, pid]);
      return json({ sdp: res.value });
    }
  }


  if (action === "answer") {
    if (req.method === "POST") {
      const { sdp, playerId } = await req.json();
      await kv.set(["answers", code, playerId], sdp, { expireIn: 60000 });
      return json({ ok: true });
    }
    if (req.method === "GET") {
      const pid = Number(url.searchParams.get("playerId"));
      const res = await kv.get(["answers", code, pid]);
      if (!res.value) return json({ sdp: null });
      await kv.delete(["answers", code, pid]);
      return json({ sdp: res.value });
    }
  }


  if (action === "ice") {
    if (req.method === "POST") {
      const { playerId, candidates, isHost } = await req.json();
      const target = isHost ? "host" : "client";
      

      for (const cand of candidates) {
        await kv.set(["ice", code, target, playerId, crypto.randomUUID()], cand, { expireIn: 60000 });
      }
      return json({ ok: true });
    }
    if (req.method === "GET") {
      const pid = Number(url.searchParams.get("playerId"));
      const isHost = url.searchParams.get("isHost") === "true";
      const target = isHost ? "host" : "client";
      
      const list = kv.list({ prefix: ["ice", code, target, pid] });
      const candidates = [];
      const keysToDelete = [];
      
      for await (const entry of list) {
        candidates.push(entry.value);
        keysToDelete.push(entry.key);
      }
      

      for (const key of keysToDelete) {
        await kv.delete(key);
      }
      
      return json({ candidates });
    }
  }

  return json({ error: "not found" }, 404);
});
