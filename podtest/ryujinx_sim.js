const dgram = require('dgram');
const WebSocket = require('ws');
const { crc32 } = require('crc'); // 需要安装 crc 库: npm install crc

// =================== 配置区域 ===================
const ESP_IP = '192.168.11.108'; // 【请修改】你的 ESP8266 IP 地址
const ESP_PORT = 26760;          // ESP8266 监听端口
const LOCAL_PORT = 26760;        // 本机监听端口 (模拟器默认端口)
const WS_PORT = 8080;            // 前端网页端口
// ===============================================

// 1. 启动 WebSocket 服务 (给网页用)
const wss = new WebSocket.Server({ port: WS_PORT });
console.log(`[WS] WebSocket server running on port ${WS_PORT}`);

// 2. 启动 UDP Socket (模拟 Ryujinx)
const client = dgram.createSocket('udp4');

// --- 核心逻辑：CRC32 算法 (DSU 协议专用) ---
// 如果不想安装 'crc' 库，可以用这个简易函数代替，但建议用库更稳
function calculateCRC(buffer) {
    // 简单的 CRC32 实现，或者直接用 npm install crc 后的 crc32(buffer)
    // 这里为了方便，假设你安装了 crc 库。如果没有，请运行 npm install crc
    return require('crc').crc32(buffer); 
}

// --- 核心逻辑：构造“请求数据”包 ---
function sendDataRequest() {
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
    client.send(packet, ESP_PORT, ESP_IP, (err) => {
        if (err) console.error(err);
        // console.log(`[UDP] Sent handshake to ${ESP_IP}`);
    });
}

// 3. 监听 UDP 消息
client.on('listening', () => {
    const address = client.address();
    console.log(`[UDP] Ryujinx Simulator listening on ${address.address}:${address.port}`);
    
    // 模拟器行为：每隔 2 秒发送一次“心跳/请求”，防止 ESP8266 断开
    setInterval(sendDataRequest, 2000);
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
        accel: { x: accX, y: accY, z: accZ },
        gyro: { pitch: gyrP, yaw: gyrY, roll: gyrR }
    };

    // 转发给网页
    const jsonStr = JSON.stringify(dataPacket);
    wss.clients.forEach(c => {
        if (c.readyState === WebSocket.OPEN) c.send(jsonStr);
    });
});

client.bind(LOCAL_PORT);