const dgram = require('dgram');
const WebSocket = require('ws');
const http = require('http');
const fs = require('fs');
const path = require('path');
const { crc32 } = require('crc');
const { performScan } = require('./scanner');

// ================= 配置 =================
const MODE = process.argv.includes('--mode=sim') ? 'SIM' : 
             process.argv.includes('--mode=scan') ? 'SCAN' : 'BRIDGE';

const PORT_WEB = 8080;
const PORT_DSU = 26760;

console.log(`[System] Starting in ${MODE} mode...`);

if (MODE === 'SCAN') {
    performScan(null); // 纯控制台扫描
    // 3.5秒后退出
    setTimeout(() => process.exit(0), 3500);
    return;
}

// ================= Web Server =================
const server = http.createServer((req, res) => {
    if (req.method === 'GET' && (req.url === '/' || req.url === '/index.html')) {
        const htmlPath = path.join(__dirname, 'index.html');
        fs.readFile(htmlPath, 'utf8', (err, data) => {
            if (err) {
                res.writeHead(500);
                res.end('Error loading index.html');
                return;
            }
            // 注入初始配置
            const defaultIP = MODE === 'BRIDGE' ? '0.0.0.0' : '';
            const html = data.replace('${ESP_IP}', defaultIP).replace('${ESP_PORT}', PORT_DSU);
            res.writeHead(200, { 'Content-Type': 'text/html; charset=utf-8' });
            res.end(html);
        });
    } else {
        res.writeHead(404);
        res.end('Not Found');
    }
});

const wss = new WebSocket.Server({ server });

// ================= UDP Logic =================
let udpSocket = dgram.createSocket('udp4');
let simInterval = null;
let targetIP = null;
let targetPort = PORT_DSU;

udpSocket.on('message', (msg, rinfo) => {
    // 校验 DSU 头
    if (msg.length < 16 || msg.toString('ascii', 0, 4) !== 'DSUS') return;

    // 解析数据
    const accX = msg.readFloatLE(76);
    const accY = msg.readFloatLE(80);
    const accZ = msg.readFloatLE(84);
    const gyrP = msg.readFloatLE(88);
    const gyrY = msg.readFloatLE(92);
    const gyrR = msg.readFloatLE(96);

    const dataPacket = {
        type: 'motion',
        accel: { x: accX, y: accY, z: accZ },
        gyro: { pitch: gyrP, yaw: gyrY, roll: gyrR }
    };

    // 广播给前端
    const jsonStr = JSON.stringify(dataPacket);
    wss.clients.forEach(c => {
        if (c.readyState === WebSocket.OPEN) c.send(jsonStr);
    });
});

udpSocket.bind(PORT_DSU, () => {
    console.log(`[UDP] Listening on 0.0.0.0:${PORT_DSU}`);
});

// ================= WebSocket Logic =================
wss.on('connection', (ws) => {
    // 发送初始状态
    ws.send(JSON.stringify({ 
        type: 'status', 
        running: (MODE === 'BRIDGE' || !!simInterval), 
        ip: MODE === 'BRIDGE' ? 'Bridge Mode' : (targetIP || ''), 
        port: PORT_DSU,
        mode: MODE
    }));

    ws.on('message', (message) => {
        try {
            const data = JSON.parse(message);
            
            if (data.cmd === 'scan') {
                performScan(ws);
            }
            
            // 仅在 SIM 模式下允许前端控制 UDP 轮询
            if (MODE === 'SIM') {
                if (data.cmd === 'start') {
                    startSimPolling(data.ip, data.port);
                    broadcastStatus();
                } else if (data.cmd === 'stop') {
                    stopSimPolling();
                    broadcastStatus();
                }
            }
        } catch (e) {
            console.error('WS Error:', e);
        }
    });
});

function startSimPolling(ip, port) {
    if (simInterval) clearInterval(simInterval);
    targetIP = ip;
    targetPort = port;
    
    const sendReq = () => {
        const packet = Buffer.alloc(100);
        packet.write('DSUS', 0);
        packet.writeUInt16LE(1001, 4);
        packet.writeUInt16LE(packet.length - 16, 6);
        packet.writeUInt32LE(0x10000002, 16); 
        packet.writeUInt32LE(0, 8);
        packet.writeUInt32LE(crc32(packet), 8);
        udpSocket.send(packet, targetPort, targetIP, (err) => {});
    };

    simInterval = setInterval(sendReq, 2000);
    sendReq(); // 立即发送一次
    console.log(`[SIM] Polling ${targetIP}:${targetPort}`);
}

function stopSimPolling() {
    if (simInterval) clearInterval(simInterval);
    simInterval = null;
    console.log('[SIM] Polling stopped');
}

function broadcastStatus() {
    const msg = JSON.stringify({ 
        type: 'status', 
        running: (MODE === 'BRIDGE' || !!simInterval), 
        ip: MODE === 'BRIDGE' ? 'Bridge Mode' : (targetIP || ''), 
        port: PORT_DSU,
        mode: MODE
    });
    wss.clients.forEach(c => { if (c.readyState === WebSocket.OPEN) c.send(msg); });
}

server.listen(PORT_WEB, () => {
    console.log(`[Web] Server started: http://localhost:${PORT_WEB}`);
});