import * as THREE from 'three';

let sc, cam, ren, gr;

export function init() {
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

export function updateUI(d) {
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
