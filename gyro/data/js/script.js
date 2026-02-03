// <button type="button" id="togglePassword">Show</button> 
// 显示密码按钮点击事件
document.getElementById("togglePassword").addEventListener("click", function () {
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


// 独立的 返回首页函数
function returnToStatusPage() {
    setTimeout(function(){window.location.href='/';}, 15000);
}
// 独立的 更新进度条动画函数 每次增加1% 直到100% 15秒内完成
function updateProgressBar(seconds) {
    var progressBar = document.querySelector('.progress');
    var secondsDisplay = document.getElementById('seconds');
    var startTime = Date.now();
    var duration = seconds * 1000;

    var interval = setInterval(function() {
        var elapsed = Date.now() - startTime;
        var progress = Math.min((elapsed / duration) * 100, 100);

        progressBar.style.width = progress + '%';
        
        if (secondsDisplay) {
            var remaining = Math.ceil(seconds - (elapsed / 1000));
            secondsDisplay.textContent = remaining > 0 ? remaining : 0;
        }
        
        if (progress >= 100) {
            clearInterval(interval);
        }
    }, 50); // 50ms刷新一次，动画更平滑
}