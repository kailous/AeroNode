
        import * as THREE from 'three';
        import { GLTFLoader } from 'three/addons/loaders/GLTFLoader.js';

        let scene, camera, renderer, coreGroup, headModel, dynamicPointer;
        let isSimulation = true;

        async function init() {
            scene = new THREE.Scene();
            scene.fog = new THREE.FogExp2(0x000810, 0.02);

            // 稍微拉近了一点相机，配合缩小的人头
            camera = new THREE.PerspectiveCamera(40, window.innerWidth / window.innerHeight, 0.1, 1000);
            camera.position.set(0, 2.5, 14);

            renderer = new THREE.WebGLRenderer({ antialias: true, alpha: true });
            renderer.setSize(window.innerWidth, window.innerHeight);
            renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2));
            document.body.appendChild(renderer.domElement);

            scene.add(new THREE.AmbientLight(0x0066ff, 0.5));
            const pLight = new THREE.PointLight(0x00ffff, 2);
            pLight.position.set(10, 10, 10);
            scene.add(pLight);

            coreGroup = new THREE.Group();
            scene.add(coreGroup);

            await loadHead();
            createHUD();
            coreGroup.add(createAxes(8));

            animate();
        }

        async function loadHead() {
            const loader = new GLTFLoader();
            const url = 'https://raw.githubusercontent.com/mrdoob/three.js/master/examples/models/gltf/LeePerrySmith/LeePerrySmith.glb';
            
            return new Promise((resolve) => {
                loader.load(url, (gltf) => {
                    headModel = gltf.scene;
                    
                    const meshes = [];
                    headModel.traverse(c => { if(c.isMesh) meshes.push(c); });

                    meshes.forEach(m => {
                        m.material = new THREE.MeshPhongMaterial({
                            color: 0x003344,
                            emissive: 0x001122,
                            transparent: true,
                            opacity: 0.4,
                            shininess: 30,
                            side: THREE.DoubleSide
                        });
                        const wire = new THREE.Mesh(m.geometry, new THREE.MeshBasicMaterial({ 
                            color: 0x00ffff, wireframe: true, transparent: true, opacity: 0.15 
                        }));
                        m.add(wire);
                    });

                    // ✅ 关键修改：缩小比例，并微调位置
                    headModel.scale.set(1.0, 1.0, 1.0);
                    headModel.position.y = -0.7; 

                    coreGroup.add(headModel);
                    resolve();
                });
            });
        }

        function createHUD() {
            const hudGroup = new THREE.Group();
            const radius = 5.5;
            const height = 5.0;

            const ringGeo = new THREE.TorusGeometry(radius, 0.005, 8, 100);
            const ringMat = new THREE.MeshBasicMaterial({ color: 0x00ffff, transparent: true, opacity: 0.2 });
            const topR = new THREE.Mesh(ringGeo, ringMat); topR.rotation.x = Math.PI/2; topR.position.y = height/2;
            const botR = new THREE.Mesh(ringGeo, ringMat); botR.rotation.x = Math.PI/2; botR.position.y = -height/2;
            hudGroup.add(topR, botR);

            const tickMat = new THREE.MeshBasicMaterial({ color: 0x00ffff, transparent: true, opacity: 0.4 });
            for (let i = 0; i < 180; i++) {
                if ((i > 20 && i < 40) || (i > 90 && i < 110)) continue;
                const angle = (i / 180) * Math.PI * 2;
                const isMajor = i % 10 === 0;
                const tick = new THREE.Mesh(new THREE.PlaneGeometry(isMajor ? 0.05 : 0.01, isMajor ? 0.6 : 0.2), tickMat);
                tick.position.set(Math.cos(angle) * radius, 0, Math.sin(angle) * radius);
                tick.rotation.y = -angle + Math.PI/2;
                hudGroup.add(tick);
            }

            dynamicPointer = new THREE.Group();
            const pointer = new THREE.Mesh(new THREE.ConeGeometry(0.12, 0.3, 3), new THREE.MeshBasicMaterial({ color: 0xff0000 }));
            pointer.position.z = radius;
            pointer.rotation.x = -Math.PI/2;
            dynamicPointer.add(pointer);
            
            hudGroup.add(dynamicPointer);
            coreGroup.add(hudGroup);
        }

        /**
         * 创建带渐隐效果的科技感十字坐标轴
         */
        function createAxes(size = 8) {
            const axesGroup = new THREE.Group();

            // 顶点着色器：将位置传递给片元
            const vertexShader = `
                varying float vDist;
                void main() {
                    vDist = length(position); // 计算顶点距离中心点的距离
                    gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1.0);
                }
            `;

            // 片元着色器：根据距离计算透明度
            const fragmentShader = `
                uniform vec3 color;
                uniform float size;
                varying float vDist;
                void main() {
                    // 距离中心越远，透明度越低
                    float alpha = 1.0 - smoothstep(0.0, size, vDist);
                    gl_FragColor = vec4(color, alpha);
                }
            `;

            const createAxis = (points, color) => {
                const geometry = new THREE.BufferGeometry().setFromPoints(points);
                const material = new THREE.ShaderMaterial({
                    uniforms: {
                        color: { value: new THREE.Color(color) },
                        size: { value: size }
                    },
                    vertexShader,
                    fragmentShader,
                    transparent: true,
                    blending: THREE.AdditiveBlending,
                    depthWrite: false
                });
                return new THREE.Line(geometry, material);
            };

            // X 轴 (左到右)
            axesGroup.add(createAxis([
                new THREE.Vector3(-size, 0, 0),
                new THREE.Vector3(0, 0, 0),
                new THREE.Vector3(size, 0, 0)
            ], 0xff0055));

            // Y 轴 (上到下)
            axesGroup.add(createAxis([
                new THREE.Vector3(0, -size, 0),
                new THREE.Vector3(0, 0, 0),
                new THREE.Vector3(0, size, 0)
            ], 0x00ff55));

            // Z 轴 (前到后)
            axesGroup.add(createAxis([
                new THREE.Vector3(0, 0, -size),
                new THREE.Vector3(0, 0, 0),
                new THREE.Vector3(0, 0, size)
            ], 0x0088ff));

            return axesGroup;
        }

        function animate() {
            requestAnimationFrame(animate);
            const t = Date.now() * 0.001;

            if (isSimulation) {
                const rot = { x: Math.sin(t*0.4)*12, y: (t*15)%360, z: Math.cos(t*0.3)*5 };
                coreGroup.rotation.x = rot.x * (Math.PI/180);
                coreGroup.rotation.y = rot.y * (Math.PI/180);
                coreGroup.rotation.z = rot.z * (Math.PI/180);
                if (dynamicPointer) dynamicPointer.rotation.y = -coreGroup.rotation.y;
                document.getElementById('val-pitch').innerText = rot.x.toFixed(1);
                document.getElementById('val-yaw').innerText = rot.y.toFixed(1);
                document.getElementById('val-roll').innerText = rot.z.toFixed(1);

                // 更新刻度尺游标 (0° 在中间)
                const toCenteredPercent = (value, range) => {
                    const pct = 50 + (value / range) * 50;
                    return Math.min(100, Math.max(0, pct));
                };
                const pitchPct = toCenteredPercent(rot.x, 90);
                const rollPct = toCenteredPercent(rot.z, 90);
                const yawSigned = ((rot.y + 180) % 360) - 180;
                const yawPct = toCenteredPercent(yawSigned, 180);
                const pitchCursor = document.getElementById('cursor-pitch');
                const rollCursor = document.getElementById('cursor-roll');
                const yawCursor = document.getElementById('cursor-yaw');
                if (pitchCursor) pitchCursor.style.left = pitchPct + "%";
                if (rollCursor) rollCursor.style.left = rollPct + "%";
                if (yawCursor) yawCursor.style.left = yawPct + "%";
            }
            renderer.render(scene, camera);
        }

        window.addEventListener('resize', () => {
            camera.aspect = window.innerWidth / window.innerHeight;
            camera.updateProjectionMatrix();
            renderer.setSize(window.innerWidth, window.innerHeight);
        });

        document.getElementById('sim-btn').onclick = () => {
            isSimulation = !isSimulation;
            document.getElementById('sim-btn').innerText = isSimulation ? "系统模拟：开启" : "系统模拟：关闭";
        };

        init();
