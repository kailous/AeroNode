// mock.js
window.mockData = () => {
    const time = Date.now() * 0.001;
    return {
        p: Math.sin(time) * 20,      // 模拟 Pitch 摆动
        r: Math.cos(time * 0.8) * 15, // 模拟 Roll 摆动
        y: (time * 50) % 360 - 180,   // 模拟 YAW 旋转
        ax: 0, ay: 0, az: 1
    };
};
console.log("Mock Mode Active: Using synthetic data");