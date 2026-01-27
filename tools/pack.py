import os
import re
import shutil
from datetime import datetime

def pack_web_config():
    # --- 1. 备份逻辑 ---
    target_file = 'WebConfig.h'
    backup_dir = 'backup'
    
    if os.path.exists(target_file):
        # 如果不存在 backup 目录则创建
        if not os.path.exists(backup_dir):
            os.makedirs(backup_dir)
            print(f"Created directory: {backup_dir}")
        
        # 生成带时间戳的备份文件名，例如 WebConfig_20240520_103000.h
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        backup_filename = f"WebConfig_{timestamp}.h"
        backup_path = os.path.join(backup_dir, backup_filename)
        
        # 执行备份（复制文件）
        shutil.copy2(target_file, backup_path)
        print(f"Backup created: {backup_path}")

    # --- 2. 读取资源文件 ---
    try:
        with open('index.html', 'r', encoding='utf-8') as f:
            html = f.read()
        with open('style.css', 'r', encoding='utf-8') as f:
            css = f.read()
        with open('app.js', 'r', encoding='utf-8') as f:
            js = f.read()
    except FileNotFoundError as e:
        print(f"Error: 找不到资源文件 - {e.filename}")
        return

    # --- 3. 注入与封装逻辑 ---
    # 打包时移除 mock 相关逻辑（URL 参数加载 mock.js）
    mock_script_pattern = r'<script>\s*const params = new URLSearchParams\(location\.search\);.*?</script>'
    html = re.sub(mock_script_pattern, '', html, flags=re.DOTALL)

    # 替换 CSS
    html = html.replace('<link rel="stylesheet" href="style.css">', f'<style>{css}</style>')
    
    # 清理并注入 JS 逻辑
    js_clean = js.replace('export ', '')
    
    # 这里的匹配模式会自动替换 index.html 中的模块化脚本块
    script_pattern = r'<script type="module">.*?</script>'
    main_script_template = """
    <script type="module">
        {js_content}
        
        init();
        setInterval(async () => {
            try {
                const r = await fetch('/data');
                updateUI(await r.json());
            } catch(e) {{}}
        }, 50);
    </script>
    """
    html = re.sub(script_pattern, main_script_template.replace('{js_content}', js_clean), html, flags=re.DOTALL)

    # 移除 HTML 中的注释以减小体积
    html = re.sub(r'', '', html, flags=re.DOTALL)
    
    # --- 4. 写入新的 WebConfig.h ---
    header_content = f"""#ifndef WEBCONFIG_H
#define WEBCONFIG_H

#include <pgmspace.h>

const char index_html[] PROGMEM = R"rawliteral(
{html}
)rawliteral";

#endif
"""

    with open(target_file, 'w', encoding='utf-8') as f:
        f.write(header_content)
    
    print(f"Successfully packed NEW {target_file}")

if __name__ == "__main__":
    pack_web_config()
