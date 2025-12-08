const http = require('http');
const WebSocket = require('ws');
const url = require('url');

const PORT = 8080;
const clients = new Map(); // clientId -> ws
const userIds = new Map(); // clientId -> userId
const userDisplayNames = new Map(); // clientId -> displayName
const otpStore = new Map();

const server = http.createServer((req, res) => {
  res.setHeader('Content-Type', 'application/json');
  res.setHeader('Access-Control-Allow-Origin', '*');
  res.setHeader('Access-Control-Allow-Methods', 'GET, POST, OPTIONS');
  res.setHeader('Access-Control-Allow-Headers', 'Content-Type');

  if (req.method === 'OPTIONS') {
    res.writeHead(200);
    res.end();
    return;
  }

  if (req.url === '/api/auth/register/phone' && req.method === 'POST') {
    let body = '';
    req.on('data', chunk => { body += chunk; });
    req.on('end', () => {
      try {
        const data = JSON.parse(body);
        const otp = Math.floor(100000 + Math.random() * 900000).toString();
        otpStore.set(data.phone, otp);
        console.log(`[OTP] ${data.phone}: ${otp}`);
        res.writeHead(200);
        res.end(JSON.stringify({ status: 'ok', otp: otp }));
      } catch (e) {
        res.writeHead(400);
        res.end(JSON.stringify({ status: 'error' }));
      }
    });
    return;
  }

  if (req.url === '/api/auth/verify/phone' && req.method === 'POST') {
    let body = '';
    req.on('data', chunk => { body += chunk; });
    req.on('end', () => {
      try {
        const data = JSON.parse(body);
        const storedOtp = otpStore.get(data.phone);
        if (storedOtp && storedOtp === data.otp) {
          const userId = 'user_' + Date.now() + '_' + Math.random().toString(36).substr(2, 9);
          const token = Math.random().toString(36).substr(2) + Math.random().toString(36).substr(2);
          console.log(`[AUTH] User created: ${userId} (${data.phone})`);
          res.writeHead(200);
          res.end(JSON.stringify({
            status: 'ok',
            user_id: userId,
            auth_token: token,
            phone: data.phone,
            display_name: data.display_name
          }));
        } else {
          res.writeHead(400);
          res.end(JSON.stringify({ status: 'error', message: 'Invalid OTP' }));
        }
      } catch (e) {
        res.writeHead(400);
        res.end(JSON.stringify({ status: 'error' }));
      }
    });
    return;
  }

  if (req.url === '/health' && req.method === 'GET') {
    res.writeHead(200);
    res.end(JSON.stringify({ status: 'healthy', clients: clients.size }));
    return;
  }

  res.writeHead(404);
  res.end(JSON.stringify({ status: 'not found' }));
});

const wss = new WebSocket.Server({ server });

// Expect token format: <userId>:<random>
// Expect token format: <userId>:<displayName>:<random>
wss.on('connection', (ws, req) => {
  const urlObj = url.parse(req.url, true);
  const token = urlObj.query.token;
  if (!token) {
    ws.close(1008, 'No token');
    return;
  }
  // Parse userId and displayName from token (assume format userId:displayName:random)
  let userId = token;
  let displayName = "";
  if (token.includes(':')) {
    const parts = token.split(':');
    userId = parts[0];
    displayName = parts.length > 1 ? parts[1] : "";
  }
  const clientId = userId;
  clients.set(clientId, ws);
  userIds.set(clientId, userId);
  userDisplayNames.set(clientId, displayName);
  console.log(`[WS] Connected: ${clientId} (${displayName})`);

  // Broadcast online user IDs and display names to all clients
  function broadcastOnlineUsers() {
    const online = Array.from(userIds.keys()).map(cid => ({ userId: userIds.get(cid), displayName: userDisplayNames.get(cid) }));
    const msg = JSON.stringify({ type: 'online_users', users: online });
    for (const wsClient of clients.values()) {
      if (wsClient.readyState === WebSocket.OPEN) {
        wsClient.send(msg);
      }
    }
  }
  broadcastOnlineUsers();

  ws.on('message', (msg) => {
    try {
      const obj = JSON.parse(msg);
      const recipient = obj.recipient;
      if (clients.has(recipient)) {
        clients.get(recipient).send(JSON.stringify({
          type: obj.type || 'chat',
          sender_id: obj.sender_id,
          message_id: obj.message_id,
          payload: obj.payload,
          timestamp: Date.now()
        }));
        console.log(`[MSG] ${obj.sender_id} → ${recipient}`);
      } else {
        console.log(`[MSG] ${recipient} offline, dropping`);
      }
    } catch (e) {
      console.log(`[WS] Error: ${e.message}`);
    }
  });

  ws.on('close', () => {
    clients.delete(clientId);
    userIds.delete(clientId);
    userDisplayNames.delete(clientId);
    broadcastOnlineUsers();
    console.log(`[WS] Disconnected: ${clientId}`);
  });

  ws.on('error', (err) => {
    console.log(`[WS] Error: ${err.message}`);
  });
});

server.listen(PORT, '0.0.0.0', () => {
  console.log(`CarrierBridge Server listening on port ${PORT}`);
  console.log(`Make sure to update Android app NetworkClient.baseUrl to your machine IP`);
});
