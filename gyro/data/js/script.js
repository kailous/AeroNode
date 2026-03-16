// Tab Switching Logic
function openTab(evt, tabName) {
    evt.preventDefault();

    // Hide all tab contents
    var tabContents = document.getElementsByClassName("tab-content");
    for (var i = 0; i < tabContents.length; i++) {
        tabContents[i].classList.remove("active");
    }

    // Remove active class from all buttons
    var tabBtns = document.getElementsByClassName("tab-btn");
    for (var i = 0; i < tabBtns.length; i++) {
        tabBtns[i].classList.remove("active");
    }

    // Show current tab and set active button
    document.getElementById(tabName).classList.add("active");
    evt.currentTarget.classList.add("active");
}

// Show/Hide Password Toggle
const toggleBtn = document.getElementById("togglePassword");
if (toggleBtn) {
    toggleBtn.addEventListener("click", function () {
        var passwordInput = document.getElementById("p");
        var icon = this.querySelector('iconpark-icon');

        if (passwordInput.type === "password") {
            passwordInput.type = "text";
            if (icon) icon.setAttribute('name', 'show');
        } else {
            passwordInput.type = "password";
            if (icon) icon.setAttribute('name', 'hide');
        }
    });
}

// 独立的 返回首页函数
function returnToStatusPage() {
    setTimeout(function () { window.location.href = '/'; }, 15000);
}

// 独立的 更新进度条动画函数 每次增加1% 直到100% 15秒内完成
function updateProgressBar(seconds) {
    var progressBar = document.querySelector('.progress');
    var secondsDisplay = document.getElementById('seconds');
    var startTime = Date.now();
    var duration = seconds * 1000;

    var interval = setInterval(function () {
        var elapsed = Date.now() - startTime;
        var progress = Math.min((elapsed / duration) * 100, 100);

        if (progressBar) {
            progressBar.style.width = progress + '%';
        }

        if (secondsDisplay) {
            var remaining = Math.ceil(seconds - (elapsed / 1000));
            secondsDisplay.textContent = remaining > 0 ? remaining : 0;
        }

        if (progress >= 100) {
            clearInterval(interval);
        }
    }, 50); // 50ms刷新一次，动画更平滑
}

// Update Slider value displays in real time
function setupSliders() {
    const sensRange = document.getElementById('sensRange');
    const sensInput = document.getElementById('sensInput');
    if (sensRange && sensInput) {
        // Slider changing Input
        sensRange.addEventListener('input', function () {
            sensInput.value = this.value;
        });
        // Input changing Slider
        sensInput.addEventListener('input', function () {
            sensRange.value = this.value;
        });
    }

    const deadRange = document.getElementById('deadRange');
    const deadInput = document.getElementById('deadInput');
    if (deadRange && deadInput) {
        deadRange.addEventListener('input', function () {
            deadInput.value = this.value;
        });
        deadInput.addEventListener('input', function () {
            deadRange.value = this.value;
        });
    }
}
document.addEventListener('DOMContentLoaded', setupSliders);

// Joystick Logic
function setupJoystick() {
    const box = document.getElementById('joystickBox');
    const knob = document.getElementById('joystickKnob');
    const gxInput = document.getElementById('gx_off');
    const gzInput = document.getElementById('gz_off');

    if (!box || !knob) return;

    // 参数范围 (最大偏移值例如 ±5.0)
    const MAX_OFFSET = 5.0;

    let isDragging = false;

    // 将 {GYROX_OFFSET} 这类可能带来的字符串尝试解析为数字（如果还没有被ESP32替换的话，就默认为0）
    let initialX = parseFloat(gxInput.value) || 0;
    let initialZ = parseFloat(gzInput.value) || 0;

    // 初始化旋钮位置
    function updateKnobPosition(xVal, zVal) {
        // xVal 和 zVal 的范围是 [-MAX_OFFSET, MAX_OFFSET]
        // 映射到 [0, 100]%
        let pctX = ((zVal + MAX_OFFSET) / (MAX_OFFSET * 2)) * 100; // Z轴影响左右 (Yaw)
        let pctY = ((-xVal + MAX_OFFSET) / (MAX_OFFSET * 2)) * 100; // X轴影响上下 (Pitch) - 注意这里取反因为屏幕Y向下为正，但我们希望推上去是正俯仰

        knob.style.left = pctX + "%";
        knob.style.top = pctY + "%";
    }

    updateKnobPosition(initialX, initialZ);

    function handleMove(e) {
        if (!isDragging) return;
        const rect = box.getBoundingClientRect();

        // 获取相对于 box 的坐标
        let x = e.clientX - rect.left;
        let y = e.clientY - rect.top;

        // 限制在盒子内部
        x = Math.max(0, Math.min(x, rect.width));
        y = Math.max(0, Math.min(y, rect.height));

        // 更新 UI
        knob.style.left = (x / rect.width * 100) + "%";
        knob.style.top = (y / rect.height * 100) + "%";

        // 反向映射回数值
        // Z 控制左右
        let zVal = ((x / rect.width) * 2 - 1) * MAX_OFFSET;
        // X 控制上下 (反转Y)
        let xVal = -((y / rect.height) * 2 - 1) * MAX_OFFSET;

        zVal = zVal.toFixed(2);
        xVal = xVal.toFixed(2);

        // 设置到隐藏并显示
        gzInput.value = zVal;
        gxInput.value = xVal;
    }

    box.addEventListener('pointerdown', (e) => {
        isDragging = true;
        box.setPointerCapture(e.pointerId); // 捕获指针，拖到框外也能响应
        handleMove(e);
    });

    box.addEventListener('pointermove', handleMove);

    box.addEventListener('pointerup', (e) => {
        isDragging = false;
        box.releasePointerCapture(e.pointerId);
    });

    // Handle Input change updating the knob
    function handleInputChange() {
        let nX = parseFloat(gxInput.value) || 0;
        let nZ = parseFloat(gzInput.value) || 0;
        // 限制最大值以防输入暴走
        nX = Math.max(-MAX_OFFSET, Math.min(nX, MAX_OFFSET));
        nZ = Math.max(-MAX_OFFSET, Math.min(nZ, MAX_OFFSET));
        updateKnobPosition(nX, nZ);
    }

    if (gxInput && gzInput) {
        gxInput.addEventListener('input', handleInputChange);
        gzInput.addEventListener('input', handleInputChange);
    }
}
document.addEventListener('DOMContentLoaded', setupJoystick);