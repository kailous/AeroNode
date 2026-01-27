#ifndef WEBCONFIG_H
#define WEBCONFIG_H

#include <pgmspace.h> 

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html lang="zh-CN">
<head>
  <meta charset="UTF-8">
  <title>Phoenix Core</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: "Microsoft YaHei", sans-serif; text-align: center; background: #1a1a1a; color: #fff; padding: 10px; margin:0;}
    h3 { color: #00e676; margin: 10px 0; letter-spacing: 1px;}
    .container { display: flex; flex-wrap: wrap; justify-content: center; max-width: 800px; margin: auto; }
    .panel { background: #2d2d2d; padding: 20px; border-radius: 10px; margin: 10px; flex: 1; min-width: 300px; box-shadow: 0 4px 10px rgba(0,0,0,0.3);}
    /* 3D Scene */
    .scene { width: 100%; height: 200px; perspective: 600px; display: flex; align-items: center; justify-content: center; }
    .cube { width: 80px; height: 80px; position: relative; transform-style: preserve-3d; transform: rotateX(0deg) rotateY(0deg); transition: transform 0.1s linear; }
    .face { position: absolute; width: 80px; height: 80px; border: 2px solid #00e676; opacity: 0.3; display: flex; align-items: center; justify-content: center; font-weight: bold; font-size: 20px; color: #00e676; background: rgba(0, 230, 118, 0.05); }
    .front { transform: translateZ(40px); } .back { transform: rotateY(180deg) translateZ(40px); }
    .right { transform: rotateY(90deg) translateZ(40px); } .left { transform: rotateY(-90deg) translateZ(40px); }
    .top { transform: rotateX(90deg) translateZ(40px); background: rgba(255, 152, 0, 0.1); border-color: #ff9800; color: #ff9800; opacity: 0.6;}
    .bottom { transform: rotateX(-90deg) translateZ(40px); }
    /* Axis */
    .axis { position: absolute; top: 50%; left: 50%; transform-style: preserve-3d; }
    .axis-x { width: 140px; height: 2px; background: #ff2d55; margin-left: -70px; margin-top: -1px; } .axis-x::after{content:"X";color:#ff2d55;position:absolute;right:-15px;top:-7px;}
    .axis-y { width: 2px; height: 140px; background: #00e676; margin-left: -1px; margin-top: -70px; } .axis-y::after{content:"Y";color:#00e676;position:absolute;bottom:-20px;left:-5px;}
    .axis-z { width: 2px; height: 140px; background: #007aff; margin-left: -1px; margin-top: -70px; transform: rotateX(90deg); } .axis-z::after{content:"Z";color:#007aff;position:absolute;bottom:-20px;left:-5px;}
    /* Inputs */
    .row { display: flex; justify-content: space-between; align-items: center; margin: 10px 0; border-bottom: 1px solid #444; padding-bottom: 5px; }
    input[type=text], input[type=password] { width: 140px; padding: 6px; background: #111; color: #fff; border: 1px solid #555; border-radius: 4px; text-align: center; }
    input[type=number] { width: 60px; padding: 5px; background: #111; color: #fff; border: 1px solid #555; border-radius: 4px; text-align: center; font-weight: bold;}
    input[type=checkbox] { transform: scale(1.5); accent-color: #00e676; }
    select { padding: 8px; background: #111; color: #00e676; border: 1px solid #555; border-radius: 4px; font-weight: bold; width: 140px; text-align: center;}
    button { background: #00e676; border: none; padding: 12px; width: 100%; font-weight: bold; cursor: pointer; border-radius: 4px; color: #000; font-size: 15px; margin-top: 15px; transition: 0.2s;}
    button:active { transform: scale(0.98); } button.calib { background: #ff9800; color: #000; }
    
    .wifi-section { border: 1px solid #00e676; border-radius: 8px; padding: 10px; margin-bottom: 15px; background: rgba(0,230,118,0.05);}
  </style>
</head>
<body>
  <h3>PHOENIX CORE</h3>
  <div class="container">
    <div class="panel">
      <div class="scene"><div class="cube" id="cube"><div class="axis axis-x"></div><div class="axis axis-y"></div><div class="axis axis-z"></div><div class="face front">前</div><div class="face back">后</div><div class="face right">右</div><div class="face left">左</div><div class="face top">上</div><div class="face bottom">下</div></div></div>
      <div id="status" style="margin-top:20px; font-family:monospace;">
         <div>X: <span id="ax" style="color:#ff2d55">0.00</span> | P: <span id="gx" style="color:#ff2d55">0.00</span></div>
         <div>Y: <span id="ay" style="color:#00e676">0.00</span> | R: <span id="gz" style="color:#00e676">0.00</span></div>
         <div>Z: <span id="az" style="color:#007aff">0.00</span> | Y: <span id="gy" style="color:#007aff">0.00</span></div>
      </div>
    </div>
    <div class="panel">
      <form action="/save" method="POST">
        <div class="wifi-section">
          <div style="color:#00e676; font-size:12px; margin-bottom:5px;">📡 WiFi 设置 (留空则不修改)</div>
          <div class="row"><label>SSID</label><input type="text" name="ssid" placeholder="%SSID%" value=""></div>
          <div class="row"><label>Pass</label><input type="password" name="pass" placeholder="******" value=""></div>
        </div>

        <div class="row"><label>📉 降噪</label><select name="dlpf"><option value="0" %SEL_0%>关闭</option><option value="1" %SEL_1%>弱</option><option value="2" %SEL_2%>平衡</option><option value="3" %SEL_3%>强</option><option value="4" %SEL_4%>极强</option><option value="6" %SEL_6%>电影级</option></select></div>
        <div class="row"><label style="color:#ff2d55">反转 Pitch</label><input type="checkbox" name="inv_gx" %CHK_GX%></div>
        <div class="row"><label style="color:#007aff">反转 Yaw</label><input type="checkbox" name="inv_gy" %CHK_GY%></div>
        <div class="row"><label style="color:#00e676">反转 Roll</label><input type="checkbox" name="inv_gz" %CHK_GZ%></div>
        <div class="row"><label style="color:#ff2d55">偏 X</label><input type="number" name="off_gx" value="%OFF_GX%"></div>
        <div class="row"><label style="color:#007aff">偏 Z</label><input type="number" name="off_gy" value="%OFF_GY%"></div>
        <div class="row"><label style="color:#00e676">偏 Y</label><input type="number" name="off_gz" value="%OFF_GZ%"></div>
        <button type="submit">保存并重启</button>
      </form>
      <form action="/calibrate" method="POST"><button type="submit" class="calib">重新校准</button></form>
    </div>
  </div>
  <script>
    setInterval(() => {
      fetch('/data').then(r=>r.json()).then(d=>{
        document.getElementById('ax').innerText=d.ax.toFixed(2); document.getElementById('ay').innerText=d.ay.toFixed(2); document.getElementById('az').innerText=d.az.toFixed(2);
        document.getElementById('gx').innerText=d.gx.toFixed(2); document.getElementById('gy').innerText=d.gy.toFixed(2); document.getElementById('gz').innerText=d.gz.toFixed(2);
        let p = Math.atan2(d.ay, d.az)*180/Math.PI; let r = Math.atan2(-d.ax, d.az)*180/Math.PI;
        document.getElementById('cube').style.transform = `rotateX(${-p-90}deg) rotateZ(${r}deg)`;
      });
    }, 100);
  </script>
</body>
</html>
)rawliteral";

#endif