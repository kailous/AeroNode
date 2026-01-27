let IS_SIMULATION = true;

let scene;
let camera;
let renderer;
let coreGroup;
let logoPlaneMat;
let orbitalRing;
let axesGroup;

init();

function init() {
    scene = createScene();
    camera = createCamera();
    renderer = createRenderer();

    addAmbientLight(scene);

    const materials = createMaterials();
    const core = createCoreModel(materials);
    coreGroup = core.coreGroup;
    orbitalRing = core.orbitalRing;

    const logo = createLogo(coreGroup);
    logoPlaneMat = logo.logoPlaneMat;

    axesGroup = addAxes(coreGroup);
    scene.add(coreGroup);

    addModelLabels({
        base: core.baseMesh,
        top: core.topMesh,
        pins: core.pinGroup,
        ring: orbitalRing,
        logo: logo.logoPlane,
        axes: axesGroup
    });

    animate();
    setupResizeHandler(camera, renderer);
}

// --- 1. 场景/相机/渲染器 ---
function createScene() {
    const scene = new THREE.Scene();
    scene.fog = new THREE.FogExp2(0x001133, 0.015);
    return scene;
}

function createCamera() {
    const camera = new THREE.PerspectiveCamera(50, window.innerWidth / window.innerHeight, 0.1, 1000);
    camera.position.set(0, 7, 11);
    camera.lookAt(0, 0, 0);
    return camera;
}

function createRenderer() {
    const renderer = new THREE.WebGLRenderer({ antialias: true, alpha: true });
    renderer.setSize(window.innerWidth, window.innerHeight);
    renderer.setPixelRatio(window.devicePixelRatio);
    renderer.setClearColor(0x000000, 1);
    document.body.appendChild(renderer.domElement);
    return renderer;
}

function addAmbientLight(scene) {
    const ambientLight = new THREE.AmbientLight(0x112233);
    scene.add(ambientLight);
    return ambientLight;
}

// --- 2. 材质 ---
function createMaterials() {
    const holoBaseMat = new THREE.MeshPhongMaterial({
        color: 0x00aaff,
        emissive: 0x0044aa,
        transparent: true,
        opacity: 0.15,
        side: THREE.DoubleSide,
        blending: THREE.AdditiveBlending,
        flatShading: true
    });
    const pinLightMat = new THREE.MeshBasicMaterial({
        color: 0x00ffff,
        transparent: true,
        opacity: 0.8,
        blending: THREE.AdditiveBlending
    });

    return { holoBaseMat, pinLightMat };
}

// --- 3. 核心模型 ---
function createCoreModel(materials) {
    const { holoBaseMat, pinLightMat } = materials;
    const coreGroup = new THREE.Group();

    const baseGeo = new THREE.BoxGeometry(2.2, 0.15, 2.2);
    const baseMesh = new THREE.Mesh(baseGeo, holoBaseMat);
    coreGroup.add(baseMesh);
    // 去掉模型网格（wireframe）

    const topGeo = new THREE.BoxGeometry(1.3, 0.15, 1.3);
    const topMesh = new THREE.Mesh(topGeo, holoBaseMat);
    topMesh.position.y = 0.15;
    coreGroup.add(topMesh);

    // ✅ 关键改动：去掉最中间的蓝色方块（coreLight）
    // （原来这里是 0.5x0.2x0.5 的 BoxGeometry）
    // --- 已移除 ---

    const pinGroup = addPins(coreGroup, pinLightMat);
    const orbitalRing = addOrbitalRing(coreGroup);

    return { coreGroup, orbitalRing, baseMesh, topMesh, pinGroup };
}

function addPins(coreGroup, pinLightMat) {
    const pinGroup = new THREE.Group();
    const pinGeo = new THREE.BoxGeometry(0.08, 0.08, 0.4);
    const pinCount = 6;
    const spacing = 1.6 / pinCount;

    for (let i = 0; i < pinCount; i++) {
        let offset = -0.8 + (i * spacing) + (spacing / 2);
        let pinL = new THREE.Mesh(pinGeo, pinLightMat);
        pinL.position.set(-1.15, 0, offset);
        pinL.rotation.z = Math.PI / 2;

        let pinR = new THREE.Mesh(pinGeo, pinLightMat);
        pinR.position.set(1.15, 0, offset);
        pinR.rotation.z = Math.PI / 2;

        let pinF = new THREE.Mesh(new THREE.BoxGeometry(0.4, 0.08, 0.08), pinLightMat);
        pinF.position.set(offset, 0, 1.15);

        let pinB = new THREE.Mesh(new THREE.BoxGeometry(0.4, 0.08, 0.08), pinLightMat);
        pinB.position.set(offset, 0, -1.15);

        pinGroup.add(pinL); pinGroup.add(pinR); pinGroup.add(pinF); pinGroup.add(pinB);
    }

    coreGroup.add(pinGroup);
    return pinGroup;
}

function addOrbitalRing(coreGroup) {
    const ringGroup = new THREE.Group();

    // 亮一些的淡蓝色发光环
    const ringRadius = 3.0;
    const glowGeo = new THREE.TorusGeometry(ringRadius, 0.02, 8, 128);
    const glowMat = new THREE.MeshBasicMaterial({
        color: 0x66ccff,
        transparent: true,
        opacity: 0.6,
        blending: THREE.AdditiveBlending
    });
    const glowRing = new THREE.Mesh(glowGeo, glowMat);
    glowRing.rotation.x = Math.PI / 2;
    ringGroup.add(glowRing);

    // 外圈刻度（一整圈）
    const outerTickMat = new THREE.MeshBasicMaterial({
        color: 0x99ddff,
        transparent: true,
        opacity: 0.9,
        blending: THREE.AdditiveBlending
    });
    const tickShortLen = 0.16;
    const tickLongLen = 0.26;
    const tickWidth = 0.02;
    const tickDepth = 0.04;
    const tickShort = new THREE.BoxGeometry(tickWidth, tickShortLen, tickDepth);
    const tickLong = new THREE.BoxGeometry(tickWidth, tickLongLen, tickDepth);

    const totalTicks = 180;
    for (let i = 0; i < totalTicks; i++) {
        const isLong = i % 10 === 0;
        const tick = new THREE.Mesh(isLong ? tickLong : tickShort, outerTickMat);
        const angle = (i / totalTicks) * Math.PI * 2;
        const radius = ringRadius + 0.08;
        tick.position.set(Math.cos(angle) * radius, 0, Math.sin(angle) * radius);
        tick.rotation.y = -angle;
        ringGroup.add(tick);
    }

    // 内圈细环 + 角度刻度
    const innerRadius = ringRadius - 0.45;
    const arcStart = -Math.PI * 0.35;
    const arcLength = Math.PI * 1.6;
    addArcSegment(ringGroup, innerRadius, 0.006, arcLength, arcStart, 0x7fd6ff, 0.5);
    addAngleTicks(ringGroup, innerRadius, arcStart, arcStart + arcLength, Math.PI / 36, 6);

    coreGroup.add(ringGroup);
    window.orbitalRing = ringGroup;
    return ringGroup;
}

function addArcSegment(group, radius, tube, arc, startAngle, color, opacity) {
    const geo = new THREE.TorusGeometry(radius, tube, 6, 96, arc);
    const mat = new THREE.MeshBasicMaterial({
        color,
        transparent: true,
        opacity,
        blending: THREE.AdditiveBlending
    });
    const mesh = new THREE.Mesh(geo, mat);
    mesh.rotation.x = Math.PI / 2;
    mesh.rotation.y = startAngle;
    group.add(mesh);
    return mesh;
}

function addAngleTicks(group, radius, startAngle, endAngle, step, majorEvery) {
    const tickMat = new THREE.MeshBasicMaterial({
        color: 0x8fd8ff,
        transparent: true,
        opacity: 0.8,
        blending: THREE.AdditiveBlending
    });
    const shortLen = 0.08;
    const longLen = 0.14;
    const height = 0.01;
    const depth = 0.02;
    const shortGeo = new THREE.BoxGeometry(shortLen, height, depth);
    const longGeo = new THREE.BoxGeometry(longLen, height, depth);

    let index = 0;
    for (let angle = startAngle; angle <= endAngle; angle += step) {
        const isMajor = index % majorEvery === 0;
        const geo = isMajor ? longGeo : shortGeo;
        const tick = new THREE.Mesh(geo, tickMat);
        tick.position.set(Math.cos(angle) * radius, 0, Math.sin(angle) * radius);
        tick.rotation.y = -angle + Math.PI / 2;
        group.add(tick);
        index++;
    }
}

// --- 4. LOGO：缩小 + 轻微发光呼吸 ---
function createLogo(coreGroup) {
    // ✅ 缩小：1.2x0.45 → 0.78x0.29（约 65%）
    const logoPlaneGeo = new THREE.PlaneGeometry(0.78, 0.29);
    const logoPlaneMat = new THREE.MeshBasicMaterial({
        transparent: true,
        opacity: 0, // 等图片加载
        blending: THREE.AdditiveBlending,
        side: THREE.DoubleSide,
        depthWrite: false,
        toneMapped: false
    });
    const logoPlane = new THREE.Mesh(logoPlaneGeo, logoPlaneMat);
    logoPlane.rotation.x = -Math.PI / 2;
    // ✅ 更贴近芯片顶面（顶层厚 0.15，顶面大约 y=0.225）
    logoPlane.position.y = 0.235;
    coreGroup.add(logoPlane);

    const img = new Image();
    img.crossOrigin = 'anonymous';
    img.src = 'img/LOGO.svg';
    img.onload = () => {
        const canvas = document.createElement('canvas');
        canvas.width = 512; canvas.height = 192;
        const ctx = canvas.getContext('2d');
        ctx.clearRect(0, 0, 512, 192);
        ctx.drawImage(img, 0, 0, 512, 192);

        const texture = new THREE.CanvasTexture(canvas);

        // ✅ 防止 mipmap 报错（尤其是你后面可能改尺寸/透明/过滤）
        texture.generateMipmaps = false;
        texture.minFilter = THREE.LinearFilter;
        texture.magFilter = THREE.LinearFilter;
        texture.wrapS = THREE.ClampToEdgeWrapping;
        texture.wrapT = THREE.ClampToEdgeWrapping;

        logoPlaneMat.map = texture;
        logoPlaneMat.opacity = 0.85; // ✅ 默认发光强度（不刺眼）
        logoPlaneMat.needsUpdate = true;
    };

    return { logoPlane, logoPlaneMat };
}

// --- 5. 坐标轴 ---
function addAxes(coreGroup) {
    const axisMatRed = new THREE.MeshBasicMaterial({ color:0xff0055, transparent:true, opacity:0.8, blending:THREE.AdditiveBlending });
    const axisMatGreen = new THREE.MeshBasicMaterial({ color:0x00ff55, transparent:true, opacity:0.8, blending:THREE.AdditiveBlending });
    const axisMatBlue = new THREE.MeshBasicMaterial({ color:0x0088ff, transparent:true, opacity:0.8, blending:THREE.AdditiveBlending });

    const axesGroup = new THREE.Group();
    axesGroup.add(createArrow('x', axisMatRed));
    axesGroup.add(createArrow('y', axisMatGreen));
    axesGroup.add(createArrow('z', axisMatBlue));
    coreGroup.add(axesGroup);
    return axesGroup;
}

function createArrow(axis, colorMat) {
    const arrowGroup = new THREE.Group();
    const line = new THREE.Mesh(new THREE.CylinderGeometry(0.03, 0.03, 4, 8), colorMat);
    line.position.y = 2.5; arrowGroup.add(line);
    const cone = new THREE.Mesh(new THREE.ConeGeometry(0.15, 0.4, 16), colorMat);
    cone.position.y = 4.5; arrowGroup.add(cone);
    if(axis === 'x') arrowGroup.rotation.z = -Math.PI / 2;
    if(axis === 'z') arrowGroup.rotation.x = Math.PI / 2;
    return arrowGroup;
}

// --- 6. 标注 ---
function addModelLabels(parts) {
    if (parts.base) addLabel(parts.base, '底座', new THREE.Vector3(0, 0.25, 0));
    if (parts.top) addLabel(parts.top, '上层', new THREE.Vector3(0, 0.2, 0));
    if (parts.pins) addLabel(parts.pins, '引脚', new THREE.Vector3(0, 0.2, 1.2));
    if (parts.ring) addLabel(parts.ring, '轨道环', new THREE.Vector3(0, 0.5, 3.2));
    if (parts.logo) addLabel(parts.logo, 'LOGO', new THREE.Vector3(0, 0.25, 0));
    if (parts.axes) addLabel(parts.axes, '坐标轴', new THREE.Vector3(0, 4.8, 0));
}

function addLabel(target, text, offset) {
    const sprite = createLabelSprite(text);
    if (offset) {
        sprite.position.copy(offset);
    } else {
        sprite.position.set(0, 0.3, 0);
    }
    target.add(sprite);
    return sprite;
}

function createLabelSprite(text) {
    const canvas = document.createElement('canvas');
    canvas.width = 256;
    canvas.height = 64;
    const ctx = canvas.getContext('2d');
    ctx.clearRect(0, 0, canvas.width, canvas.height);
    ctx.fillStyle = 'rgba(0, 0, 0, 0.35)';
    ctx.fillRect(0, 0, canvas.width, canvas.height);
    ctx.strokeStyle = 'rgba(0, 255, 255, 0.8)';
    ctx.lineWidth = 2;
    ctx.strokeRect(1, 1, canvas.width - 2, canvas.height - 2);
    ctx.font = '24px Arial';
    ctx.textAlign = 'center';
    ctx.textBaseline = 'middle';
    ctx.fillStyle = 'rgba(255, 255, 255, 0.95)';
    ctx.fillText(text, canvas.width / 2, canvas.height / 2);

    const texture = new THREE.CanvasTexture(canvas);
    texture.minFilter = THREE.LinearFilter;
    texture.magFilter = THREE.LinearFilter;

    const material = new THREE.SpriteMaterial({
        map: texture,
        transparent: true,
        depthWrite: false
    });
    const sprite = new THREE.Sprite(material);
    sprite.scale.set(1.8, 0.45, 1);
    return sprite;
}

function createTextSprite(text, color, opacity) {
    const canvas = document.createElement('canvas');
    canvas.width = 256;
    canvas.height = 128;
    const ctx = canvas.getContext('2d');
    ctx.clearRect(0, 0, canvas.width, canvas.height);
    ctx.font = '32px Arial';
    ctx.textAlign = 'center';
    ctx.textBaseline = 'middle';
    let fillStyle = color;
    if (typeof color === 'number') {
        fillStyle = `#${color.toString(16).padStart(6, '0')}`;
    }
    ctx.fillStyle = fillStyle || 'rgba(160, 220, 255, 0.9)';
    ctx.globalAlpha = opacity === undefined ? 0.9 : opacity;
    ctx.fillText(text, canvas.width / 2, canvas.height / 2);

    const texture = new THREE.CanvasTexture(canvas);
    texture.minFilter = THREE.LinearFilter;
    texture.magFilter = THREE.LinearFilter;

    const material = new THREE.SpriteMaterial({
        map: texture,
        transparent: true,
        depthWrite: false
    });
    const sprite = new THREE.Sprite(material);
    return sprite;
}

// --- 7. 动画与逻辑 ---
function toggleSim() {
    IS_SIMULATION = !IS_SIMULATION;
    const btn = document.getElementById('sim-btn');
    if (IS_SIMULATION) {
        btn.classList.add('active');
        btn.innerText = "🟢 SIMULATION DATA STREAM";
    } else {
        btn.classList.remove('active');
        btn.innerText = "🔴 AWAITING REALLINK...";
    }
}

function getMockData() {
    const time = Date.now() * 0.001;
    let movement = Math.sin(time) * Math.cos(time*0.5) * 30;
    return { gx: movement, gy: time*10 % 360, gz: Math.cos(time)*10 };
}

function updateState() {
    if (IS_SIMULATION) {
        applyRotation(getMockData());
    } else {
        fetch('/data').then(r => r.json()).then(data => applyRotation(data)).catch(e => {});
    }
}

function applyRotation(data) {
    document.getElementById('val-pitch').innerText = data.gx.toFixed(1);
    document.getElementById('val-yaw').innerText = data.gy.toFixed(1);
    document.getElementById('val-roll').innerText = data.gz.toFixed(1);
    coreGroup.rotation.x = data.gx * (Math.PI / 180);
    coreGroup.rotation.y = data.gy * (Math.PI / 180);
    coreGroup.rotation.z = data.gz * (Math.PI / 180);
}

function animate() {
    requestAnimationFrame(animate);
    updateState();

    // ✅ LOGO 轻微呼吸（加载完贴图后才生效）
    if (logoPlaneMat && logoPlaneMat.map) {
        const t = performance.now() * 0.002;
        logoPlaneMat.opacity = 0.78 + Math.sin(t) * 0.08; // 0.70~0.86
    }

    if (orbitalRing) orbitalRing.rotation.y += 0.003;

    renderer.render(scene, camera);
}

function setupResizeHandler(camera, renderer) {
    window.addEventListener('resize', () => {
        camera.aspect = window.innerWidth / window.innerHeight;
        camera.updateProjectionMatrix();
        renderer.setSize(window.innerWidth, window.innerHeight);
    });
}
