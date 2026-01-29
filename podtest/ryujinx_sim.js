const dgram = require('dgram');
const WebSocket = require('ws');
const http = require('http');
const fs = require('fs');
const path = require('path');
const { crc32 } = require('crc'); // 需要安装 crc 库: npm install crc

// =================== 配置区域 ===================
let ESP_IP = '';                 // 默认 ESP8266 IP 地址
let ESP_PORT = 26760;            // 默认 ESP8266 监听端口
const LOCAL_PORT = 26760;        // 本机监听端口 (模拟器默认端口)
const WEB_PORT = 8080;           // Web 服务端口
// ===============================================

// 全局状态
let client = null; // UDP Socket
let handshakeInterval = null;
let isRunning = false;

// ================= Web 服务器 (UI + WebSocket) =================

// 1. 启动 HTTP 服务 (读取 index.html)
const server = http.createServer((req, res) => {
    if (req.method === 'GET' && (req.url === '/' || req.url === '/index.html')) {
        fs.readFile(path.join(__dirname, 'index.html'), 'utf8', (err, data) => {
            if (err) {
                res.writeHead(500);
                res.end('Error loading index.html');
                return;
            }
            // 简单的模板替换，将 JS 中的配置注入到 HTML
            const html = data.replace('${ESP_IP}', ESP_IP).replace('${ESP_PORT}', ESP_PORT);
            res.writeHead(200, { 'Content-Type': 'text/html; charset=utf-8' });
            res.end(html);
        });
    } else {
        res.writeHead(404);
        res.end('Not Found');
    }
});

// 3. 启动 WebSocket 服务 (挂载在 HTTP Server 上)
const wss = new WebSocket.Server({ server });

wss.on('connection', (ws) => {
    // 连接时发送当前状态
    ws.send(JSON.stringify({ type: 'status', running: isRunning, ip: ESP_IP, port: ESP_PORT }));

    ws.on('message', (message) => {
        try {
            const data = JSON.parse(message);
            if (data.cmd === 'start') {
                startUdpService(data.ip, data.port);
            } else if (data.cmd === 'stop') {
                stopUdpService();
            }
        } catch (e) {
            console.error('WS Error:', e);
        }
    });
});

server.listen(WEB_PORT, () => {
    console.log(`[Web] 管理页面已启动: http://localhost:${WEB_PORT}`);
});

// --- 核心逻辑：CRC32 算法 (DSU 协议专用) ---
// 如果不想安装 'crc' 库，可以用这个简易函数代替，但建议用库更稳
function calculateCRC(buffer) {
    return crc32(buffer); 
}

// --- 核心逻辑：构造“请求数据”包 ---
function sendDataRequest() {
    if (!client) return;

    // DSU 协议头: 'D', 'S', 'U', 'S', Version(E9,03), Length(16), 0, CRC(4 bytes), ID(4 bytes)...
    const packet = Buffer.alloc(100); 

    // Header "DSUS"
    packet.write('DSUS', 0);
    // Version 1001
    packet.writeUInt16LE(1001, 4);
    // Length (Packet size without header)
    packet.writeUInt16LE(packet.length - 16, 6);
    
    // Packet Type: 0x02 (UC_PACKET_DATA - 请求数据)
    // 这就是告诉 ESP8266 "快给我发数据" 的开关
    packet.writeUInt32LE(0x10000002, 16); // Type 2 + Slot 0

    // 可以在这里填入本机 MAC 地址，但通常全 0 也可以
    
    // 计算 CRC32 并填入 Byte 8-11
    // 注意：CRC 计算通常是针对整个包的（除了 CRC 字段本身），或者部分。
    // 在 DSU 协议中，CRC 覆盖整个包，计算时 CRC 字段填 0。
    packet.writeUInt32LE(0, 8); // 先清零 CRC 区域
    const crcValue = calculateCRC(packet);
    packet.writeUInt32LE(crcValue, 8); // 填入计算好的 CRC

    // 发送给 ESP8266
    try {
        client.send(packet, ESP_PORT, ESP_IP, (err) => {
            if (err) console.error(err);
        });
    } catch (e) {
        console.error('[UDP Send Error]', e);
        stopUdpService();
    }
}

// ================= UDP 服务控制 =================

function startUdpService(ip, port) {
    if (isRunning) stopUdpService();

    ESP_IP = ip;
    ESP_PORT = port;

    client = dgram.createSocket('udp4');

    client.on('listening', () => {
        const address = client.address();
        console.log(`[UDP] Ryujinx Simulator listening on ${address.address}:${address.port}`);
        
        isRunning = true;
        broadcastStatus();

        // 模拟器行为：每隔 2 秒发送一次“心跳/请求”，防止 ESP8266 断开
        handshakeInterval = setInterval(sendDataRequest, 2000);
        sendDataRequest(); // 立即发送一次
    });

    client.on('message', (msg, rinfo) => {
        // 过滤：只处理 DSU 数据包
        if (msg.length < 16 || msg.toString('ascii', 0, 4) !== 'DSUS') return;

        // 解析数据 (和之前一样)
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

        // 转发给网页
        const jsonStr = JSON.stringify(dataPacket);
        wss.clients.forEach(c => {
            if (c.readyState === WebSocket.OPEN) c.send(jsonStr);
        });
    });

    client.on('error', (err) => {
        console.error('[UDP Error]', err);
        stopUdpService();
    });

    client.bind(LOCAL_PORT);
}

function stopUdpService() {
    if (handshakeInterval) clearInterval(handshakeInterval);
    if (client) {
        client.close();
        client = null;
    }
    isRunning = false;
    broadcastStatus();
    console.log('[UDP] Service Stopped');
}

function broadcastStatus() {
    const msg = JSON.stringify({ type: 'status', running: isRunning, ip: ESP_IP, port: ESP_PORT });
    wss.clients.forEach(c => {
        if (c.readyState === WebSocket.OPEN) c.send(msg);
    });
}