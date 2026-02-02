const dgram = require('dgram');
const { crc32 } = require('crc');

const SCAN_PORT = 26760;

function performScan(ws) {
    const scanner = dgram.createSocket('udp4');
    let found = false;
    
    console.log('[Scanner] Starting scan...');
    
    scanner.on('listening', () => {
        scanner.setBroadcast(true);
        const packet = Buffer.alloc(100);
        // Header "DSUS"
        packet.write('DSUS', 0);
        // Version 1001
        packet.writeUInt16LE(1001, 4);
        // Length
        packet.writeUInt16LE(packet.length - 16, 6);
        // Type: 0x10000002 (请求数据)
        packet.writeUInt32LE(0x10000002, 16); 
        // CRC
        packet.writeUInt32LE(0, 8);
        packet.writeUInt32LE(crc32(packet), 8);

        scanner.send(packet, SCAN_PORT, '255.255.255.255', (err) => {
            if (err) console.error('[Scanner] Broadcast error:', err);
        });
    });

    scanner.on('message', (msg, rinfo) => {
        if (msg.length >= 4 && msg.toString('ascii', 0, 4) === 'DSUS') {
            found = true;
            if (ws && ws.readyState === 1) { // WebSocket.OPEN = 1
                ws.send(JSON.stringify({ type: 'scan_result', ip: rinfo.address, port: rinfo.port }));
            } else {
                console.log(`[Scanner] Found device: ${rinfo.address}:${rinfo.port}`);
            }
        }
    });

    scanner.bind();
    
    setTimeout(() => {
        try { scanner.close(); } catch(e){}
        if (!found && ws && ws.readyState === 1) {
            ws.send(JSON.stringify({ type: 'scan_timeout' }));
        }
    }, 3000);
}

module.exports = { performScan };