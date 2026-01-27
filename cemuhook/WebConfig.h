#ifndef WEBCONFIG_H
#define WEBCONFIG_H

#include <pgmspace.h>

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="zh-CN">
<head>
    <meta charset="UTF-8">
    <title>PHOENIX CORE | MONO</title>
    <script type="importmap">{ "imports": { "three": "https://unpkg.com/three@0.160.0/build/three.module.js" } }</script>
    <style>
        :root { --fg: #ffffff; --bg: #000000; --dim: #333333; }
        body { margin: 0; overflow: hidden; background: var(--bg); font-family: "Inter", "Helvetica", sans-serif; color: var(--fg); text-transform: uppercase; }
        
        /* HUD 仪表 */
        #ui { position: absolute; inset: 0; pointer-events: none; padding: 50px; z-index: 10; }
        .header { font-size: 12px; letter-spacing: 4px; margin-bottom: 40px; opacity: 0.5; }
        .data-row { margin-bottom: 30px; }
        .label { font-size: 10px; letter-spacing: 2px; margin-bottom: 5px; opacity: 0.4; }
        .val { font-size: 36px; font-weight: 200; font-family: 'Courier New', monospace; }
        .track { position: relative; height: 1px; background: var(--dim); margin-top: 10px; width: 200px; }
        .cursor { position: absolute; top: -5px; width: 2px; height: 11px; background: var(--fg); transform: translateX(-1px); transition: left 0.1s ease-out; }

        /* 配置面板 */
        .panel { 
            position: fixed; right: 40px; top: 50%; transform: translateY(-50%); width: 280px;
            background: var(--bg); padding: 40px; border: 1px solid var(--dim);
            pointer-events: auto; transition: 0.3s;
        }
        .panel.collapsed { right: -400px; opacity: 0; }
        .section { font-size: 9px; color: var(--dim); margin: 25px 0 10px; letter-spacing: 2px; }
        .row { display: flex; justify-content: space-between; align-items: center; margin-bottom: 12px; font-size: 12px; }
        
        input, select { background: none; border: 1px solid var(--dim); color: var(--fg); padding: 5px; outline: none; font-family: inherit; font-size: 11px; }
        input:focus { border-color: var(--fg); }
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
</head>
<body>
    <div id="ui">
        <div class="header">PHOENIX CORE // SYSTEM LIVE</div>
        
        <div class="data-row">
            <div class="label">PITCH</div>
            <div id="vp" class="val">0.0</div>
            <div class="track"><div id="cp" class="cursor"></div></div>
        </div>

        <div class="data-row">
            <div class="label">ROLL</div>
            <div id="vr" class="val">0.0</div>
            <div class="track"><div id="cr" class="cursor"></div></div>
        </div>

        <div class="data-row">
            <div class="label">YAW</div>
            <div id="vy" class="val">0.0</div>
            <div class="track"><div id="cy" class="cursor"></div></div>
        </div>
    </div>

    <div class="panel collapsed" id="side">
        <div style="display:flex; justify-content:space-between; font-size:12px; margin-bottom:20px; letter-spacing:2px;">
            <span>CONFIG</span>
            <span onclick="setS()" style="cursor:pointer; opacity:0.5">CLOSE</span>
        </div>
        <form action="/save" method="POST">
            <div class="section">NETWORK</div>
            <div class="row"><span>SSID</span><input type="text" name="ssid" value="%SSID%" style="width:120px"></div>
            
            <div class="section">FILTERING</div>
            <div class="row"><span>DLPF</span>
                <select name="dlpf" style="width:132px">
                    <option value="0" %SEL_0%>OFF</option>
                    <option value="2" %SEL_2%>MID</option>
                    <option value="6" %SEL_6%>HIGH</option>
                </select>
            </div>

            <div class="section">INVERSION</div>
            <div class="row"><span>PITCH (X)</span><label class="switch"><input type="checkbox" name="inv_gx" %CHK_GX%><span class="slider"></span></label></div>
            <div class="row"><span>ROLL (Y)</span><label class="switch"><input type="checkbox" name="inv_gy" %CHK_GY%><span class="slider"></span></label></div>
            <div class="row"><span>YAW (Z)</span><label class="switch"><input type="checkbox" name="inv_gz" %CHK_GZ%><span class="slider"></span></label></div>

            <div class="section">OFFSETS</div>
            <div class="row"><span>X</span><input type="number" name="off_gx" value="%OFF_GX%"></div>
            <div class="row"><span>Y</span><input type="number" name="off_gy" value="%OFF_GY%"></div>
            <div class="row"><span>Z</span><input type="number" name="off_gz" value="%OFF_GZ%"></div>

            <button type="submit" class="btn">APPLY CHANGES</button>
        </form>
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
            const mat = new THREE.LineBasicMaterial({ color: 0xffffff, transparent: true, opacity: 0.2 });
            
            const createLine = (pts) => {
                const geo = new THREE.BufferGeometry().setFromPoints(pts);
                return new THREE.Line(geo, mat);
            };
            gr.add(createLine([new THREE.Vector3(-10,0,0), new THREE.Vector3(10,0,0)]));
            gr.add(createLine([new THREE.Vector3(0,-10,0), new THREE.Vector3(0,10,0)]));
            gr.add(createLine([new THREE.Vector3(0,0,-10), new THREE.Vector3(0,0,10)]));
            sc.add(gr);

            animate();
            setInterval(sync, 50);
        }

        async function sync() {
            try {
                const r = await fetch('/data');
                const d = await r.json();
                
                let p = d.p || 0; 
                let rl = d.r || 0; 
                let yw = d.y || 0;

                const rad = 0.017453;
                gr.rotation.set(p * rad, yw * rad, rl * rad);
                
                document.getElementById('vp').innerText = p.toFixed(1);
                document.getElementById('vr').innerText = rl.toFixed(1);
                document.getElementById('vy').innerText = yw.toFixed(1);
                
                // 修正后的游标映射：所有轴均基于 -180~180 逻辑
                document.getElementById('cp').style.left = (50 + p/1.8) + "%";
                document.getElementById('cr').style.left = (50 + rl/1.8) + "%";
                document.getElementById('cy').style.left = (50 + yw/3.6) + "%";
            } catch(e){}
        }

        function animate() { requestAnimationFrame(animate); ren.render(sc, cam); }
        window.setS = () => document.getElementById('side').classList.toggle('collapsed');
        window.addEventListener('resize', () => { cam.aspect = window.innerWidth/window.innerHeight; cam.updateProjectionMatrix(); ren.setSize(window.innerWidth, window.innerHeight); });
        init();
    </script>
</body>
</html>
)rawliteral";

#endif