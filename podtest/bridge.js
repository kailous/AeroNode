const dgram = require('dgram');
const WebSocket = require('ws');

// ================= 配置 =================
const DSU_PORT = 26760; // 接收 ESP8266 数据
const WS_PORT = 8080;   // 发送给网页

// ================= WebSocket 服务器 =================
const wss = new WebSocket.Server({ port: WS_PORT });
console.log(`[WS] WebSocket server started on port ${WS_PORT}`);

// ================= UDP 服务器 (Cemuhook) =================
const udpServer = dgram.createSocket('udp4');

udpServer.on('listening', () => {
    const address = udpServer.address();
    console.log(`[UDP] Listening for DSU packets on ${address.address}:${address.port}`);
});

udpServer.on('message', (msg, rinfo) => {
    // 1. 简单的协议校验 (Magic String: "DSUS")
    // D:68, S:83, U:85, S:83
    if (msg[0] !== 0x44 || msg[1] !== 0x53 || msg[2] !== 0x55 || msg[3] !== 0x53) return;

    // 2. 按照你的 ESP8266 代码解析数据
    // 注意：数据是 Little Endian (小端序)
    
    // Accel (Bytes 76-87)
    const accX = msg.readFloatLE(76);
    const accY = msg.readFloatLE(80);
    const accZ = msg.readFloatLE(84);

    // Gyro (Bytes 88-99)
    const gyrP = msg.readFloatLE(88);
    const gyrY = msg.readFloatLE(92);
    const gyrR = msg.readFloatLE(96);

    // 3. 构造要发给网页的 JSON 对象
    const dataPacket = {
        type: 'motion',
        accel: { x: accX, y: accY, z: accZ },
        gyro: { pitch: gyrP, yaw: gyrY, roll: gyrR },
        timestamp: Date.now()
    };

    // 4. 广播给所有打开的网页
    const jsonStr = JSON.stringify(dataPacket);
    wss.clients.forEach(client => {
        if (client.readyState === WebSocket.OPEN) {
            client.send(jsonStr);
        }
    });
});

udpServer.bind(DSU_PORT);