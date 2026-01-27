// PACKED_VERSION: 20260127_193447
#ifndef WEBCONFIG_H
#define WEBCONFIG_H

#include <pgmspace.h>

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="zh-CN">
<head>
    <meta charset="UTF-8">
    <title>PHOENIX CORE | MONO</title>
    <style>:root { --fg: #ffffff; --bg: #000000; --dim: #333333; --min: #c5c5c5; --pitch: #FF7274; --roll: #80ed99; --yaw: #ffd60a; }
body { margin: 0; overflow: hidden; background: var(--bg); font-family: "Inter", "Helvetica", sans-serif; color: var(--fg); text-transform: uppercase; }

/* HUD 仪表 */
#ui { position: absolute; inset: 0; pointer-events: none; padding: 50px; z-index: 10; }
.header { font-size: 12px; letter-spacing: 4px; margin-bottom: 40px; opacity: 0.5; }
.data-row { margin-bottom: 30px; }
.label { font-size: 10px; letter-spacing: 2px; margin-bottom: 5px; opacity: 1; }
.val { font-size: 36px; font-weight: 200; font-family: 'Courier New', monospace; }
.track { position: relative; height: 1px; background: var(--dim); margin-top: 10px; width: 200px; }
.cursor { position: absolute; top: -4px; width: 1.5px; height: 9px; background: var(--fg); transform: translateX(-1px); transition: left 0.1s ease-out; }
.label.pitch { color: var(--pitch); }
.label.roll { color: var(--roll); }
.label.yaw { color: var(--yaw); }
.cursor.pitch { background: var(--pitch); box-shadow: 0 0 8px var(--pitch), 0 0 18px var(--pitch), 0 0 32px var(--pitch), 0 0 48px rgba(255, 114, 116, 0.8); border-radius: 6px; }
.cursor.roll { background: var(--roll); box-shadow: 0 0 8px var(--roll), 0 0 18px var(--roll), 0 0 32px var(--roll), 0 0 48px rgba(128, 237, 153, 0.8); border-radius: 6px; }
.cursor.yaw { background: var(--yaw); box-shadow: 0 0 8px var(--yaw), 0 0 18px var(--yaw), 0 0 32px var(--yaw), 0 0 48px rgba(255, 214, 10, 0.8); border-radius: 6px; }



/* 配置面板 */
.panel { 
    position: fixed; right: 40px; top: 50%; transform: translateY(-50%); width: 280px;
    background: var(--bg); padding: 40px; border: 1px solid var(--dim);
    pointer-events: auto; transition: 0.3s;
}
.panel.collapsed { right: -400px; opacity: 0; }
.section { font-size: 9px; color: var(--min); margin: 25px 0 10px; letter-spacing: 2px; }
.row { display: flex; justify-content: space-between; align-items: center; margin-bottom: 12px; font-size: 12px; }

input, select {
    background-color: rgba(255, 255, 255, 0.02);
    border: 1px solid var(--dim);
    color: var(--fg);
    padding: 5px 6px;
    outline: none;
    font-family: inherit;
    font-size: 11px;
    letter-spacing: 1px;
    transition: border-color 0.2s ease, background-color 0.2s ease;
}
input:focus, select:focus { border-color: var(--fg); background-color: rgba(255, 255, 255, 0.04); }
input:hover, select:hover { border-color: #4a4a4a; }

select {
    appearance: none;
    -webkit-appearance: none;
    -moz-appearance: none;
    padding-right: 26px;
    background-image:
        linear-gradient(45deg, transparent 50%, var(--fg) 50%),
        linear-gradient(135deg, var(--fg) 50%, transparent 50%),
        linear-gradient(to right, var(--dim), var(--dim));
    background-position:
        calc(100% - 14px) 50%,
        calc(100% - 9px) 50%,
        calc(100% - 24px) 50%;
    background-size: 5px 5px, 5px 5px, 1px 14px;
    background-repeat: no-repeat;
}
select option { background: var(--bg); color: var(--fg); }
.btn { width: 100%; padding: 15px; background: var(--fg); color: var(--bg); border: none; font-weight: bold; cursor: pointer; margin-top: 20px; font-size: 11px; letter-spacing: 2px; }

.toggle { position: fixed; bottom: 40px; right: 40px; width: 44px; height: 44px; border: 1px solid var(--dim); display: flex; align-items: center; justify-content: center; cursor: pointer; pointer-events: auto; transition: 0.2s; }
.toggle:hover { border-color: var(--fg); }

.switch { position: relative; width: 30px; height: 14px; }
.switch input { opacity: 0; width: 0; height: 0; }
.slider { position: absolute; cursor: pointer; inset: 0; border: 1px solid var(--dim); transition: .2s; }
.slider:before { position: absolute; content: ""; height: 8px; width: 8px; left: 2px; bottom: 2px; background: var(--dim); transition: .2s; }
input:checked + .slider { border-color: var(--fg); }
input:checked + .slider:before { transform: translateX(16px); background: var(--fg); }
</style>
    <script type="importmap">{ "imports": { "three": "https://unpkg.com/three@0.160.0/build/three.module.js" } }</script>
</head>
<body>
    <div id="ui">
        <div class="header">PHOENIX CORE // SYSTEM LIVE // 系统在线</div>
        <div class="data-row"><div class="label pitch">PITCH / 俯仰</div><div id="vp" class="val">0.0</div><div class="track"><div id="cp" class="cursor pitch"></div></div></div>
        <div class="data-row"><div class="label roll">ROLL / 横滚</div><div id="vr" class="val">0.0</div><div class="track"><div id="cr" class="cursor roll"></div></div></div>
        <div class="data-row"><div class="label yaw">YAW / 偏航</div><div id="vy" class="val">0.0</div><div class="track"><div id="cy" class="cursor yaw"></div></div></div>
    </div>

    <div class="panel" id="side">
        <div style="display:flex; justify-content:space-between; font-size:12px; margin-bottom:20px; letter-spacing:2px;">
            <span>CONFIG / 配置</span>
            <span onclick="setS()" style="cursor:pointer; opacity:0.5">CLOSE / 关闭</span>
        </div>
        
        <form action="/save" method="POST">
            <div class="section">NETWORK / 网络</div>
            <div class="row"><span>SSID / 名称</span><input type="text" name="ssid" value="%SSID%" style="width:120px"></div>
            <div class="row"><span>PASS / 密码</span><input id="wifi-pass" type="password" name="pass" placeholder="LEAVE BLANK TO KEEP" style="width:120px"></div>
            <div class="row"><span>SHOW PASS / 明文</span><label class="switch"><input id="toggle-pass" type="checkbox"><span class="slider"></span></label></div>
            <div class="section">FILTERING / 滤波</div>
            <div class="row"><span>DLPF / 低通</span>
                <select name="dlpf" style="width:132px">
                    <option value="0" %SEL_0%>OFF / 关闭</option>
                    <option value="2" %SEL_2%>MID / 中</option>
                    <option value="6" %SEL_6%>HIGH / 高</option>
                </select>
            </div>

            <div class="section">INVERSION / 反向</div>
            <div class="row"><span>PITCH (X) / 俯仰</span><label class="switch"><input type="checkbox" name="inv_gx" %CHK_GX%><span class="slider"></span></label></div>
            <div class="row"><span>ROLL (Y) / 横滚</span><label class="switch"><input type="checkbox" name="inv_gy" %CHK_GY%><span class="slider"></span></label></div>
            <div class="row"><span>YAW (Z) / 偏航</span><label class="switch"><input type="checkbox" name="inv_gz" %CHK_GZ%><span class="slider"></span></label></div>

            <div class="section">OFFSETS / 偏置</div>
            <div class="row"><span>X / 俯仰</span><input type="number" name="off_gx" value="%OFF_GX%"></div>
            <div class="row"><span>Y / 横滚</span><input type="number" name="off_gy" value="%OFF_GY%"></div>
            <div class="row"><span>Z / 偏航</span><input type="number" name="off_gz" value="%OFF_GZ%"></div>

            <button type="submit" class="btn">APPLY / 应用</button>
        </form>

        <div class="section">CALIBRATION / 校准</div>
        <button type="button" class="btn" onclick="fetch('/calibrate')">RESET GYRO / 陀螺仪重置</button>
    </div>
    
    <div class="toggle" onclick="setS()">+</div>

    

    
    <script type="module">
        import * as THREE from 'three';

let sc, cam, ren, gr;

function init() {
    sc = new THREE.Scene();
    cam = new THREE.PerspectiveCamera(40, window.innerWidth/window.innerHeight, 0.1, 1000);
    cam.position.z = 18;
    
    ren = new THREE.WebGLRenderer({ antialias: true, alpha: true });
    ren.setSize(window.innerWidth, window.innerHeight);
    ren.setPixelRatio(window.devicePixelRatio);
    document.body.appendChild(ren.domElement);

    gr = new THREE.Group();
    const rootStyle = getComputedStyle(document.documentElement);
    const pitchColor = rootStyle.getPropertyValue('--pitch').trim() || '#FF7274';
    const rollColor = rootStyle.getPropertyValue('--roll').trim() || '#80ed99';
    const yawColor = rootStyle.getPropertyValue('--yaw').trim() || '#ffd60a';
    const createLine = (pts, color) => {
        const geo = new THREE.BufferGeometry().setFromPoints(pts);
        const mat = new THREE.LineBasicMaterial({ color, transparent: true, opacity: 0.5 });
        return new THREE.Line(geo, mat);
    };
    // 轴颜色对应：Pitch->X，Roll->Y，Yaw->Z
    gr.add(createLine([new THREE.Vector3(-10,0,0), new THREE.Vector3(10,0,0)], pitchColor));
    gr.add(createLine([new THREE.Vector3(0,-10,0), new THREE.Vector3(0,10,0)], rollColor));
    gr.add(createLine([new THREE.Vector3(0,0,-10), new THREE.Vector3(0,0,10)], yawColor));

    // 平面圆环（位于 Y-Z 平面）
    const ringGeo = new THREE.RingGeometry(6.2, 6.35, 64);
    const ringMat = new THREE.MeshBasicMaterial({
        color: 0xffffff,
        transparent: true,
        opacity: 0.12,
        side: THREE.DoubleSide
    });
    const ring = new THREE.Mesh(ringGeo, ringMat);
    ring.rotation.y = Math.PI / 2;
    gr.add(ring);
    sc.add(gr);

    animate();
}

function updateUI(d) {
    let p = d.p || 0, rl = d.r || 0, yw = d.y || 0;
    const rad = 0.017453;
    // 对应关系：Pitch -> X 轴，Roll -> Y 轴，Yaw -> Z 轴
    gr.rotation.set(p * rad, rl * rad, yw * rad);
    
    document.getElementById('vp').innerText = p.toFixed(1);
    document.getElementById('vr').innerText = rl.toFixed(1);
    document.getElementById('vy').innerText = yw.toFixed(1);
    
    document.getElementById('cp').style.left = (50 + p/1.8) + "%";
    document.getElementById('cr').style.left = (50 + rl/1.8) + "%";
    document.getElementById('cy').style.left = (50 + yw/3.6) + "%";
}

function animate() { requestAnimationFrame(animate); ren.render(sc, cam); }

window.setS = () => document.getElementById('side').classList.toggle('collapsed');
window.addEventListener('resize', () => { 
    cam.aspect = window.innerWidth/window.innerHeight; 
    cam.updateProjectionMatrix(); 
    ren.setSize(window.innerWidth, window.innerHeight); 
});

        
        init();
        setInterval(async () => {
            try {
                const r = await fetch('/data');
                updateUI(await r.json());
            } catch(e) {{}}
        }, 50);
    </script>
    
    </body>
</html>

)rawliteral";

#endif
