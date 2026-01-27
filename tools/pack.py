import os
import re

def pack_web_config():
    # 1. 读取各文件内容
    with open('index.html', 'r', encoding='utf-8') as f:
        html = f.read()
    with open('style.css', 'r', encoding='utf-8') as f:
        css = f.read()
    with open('app.js', 'r', encoding='utf-8') as f:
        js = f.read()

    # 2. 将 CSS 和 JS 注入 HTML (为了方便 ESP8266 一次性下发)
    # 替换 <link rel="stylesheet" href="style.css">
    html = html.replace('<link rel="stylesheet" href="style.css">', f'<style>{css}</style>')
    
    # 提取 JS 内容并替换模块导入 (ESP8266 不方便处理多文件 import)
    # 我们把 app.js 的 export 去掉，直接内联到 script 标签中
    js_clean = js.replace('export ', '')
    
    # 寻找主逻辑 script 块并合并
    script_pattern = r'<script type="module">.*?</script>'
    main_script = """
    <script type="module">
        {js_content}
        
        init();
        setInterval(async () => {
            try {
                const r = await fetch('/data');
                updateUI(await r.json());
            } catch(e){}
        }, 50);
    </script>
    """
    html = re.sub(script_pattern, main_script.replace('{js_content}', js_clean), html, flags=re.DOTALL)

    # 3. 移除 HTML 注释和多余空格 (简单压缩)
    html = re.sub(r'', '', html, flags=re.DOTALL)
    
    # 4. 封装成 C++ Header
    header_content = f"""#ifndef WEBCONFIG_H
#define WEBCONFIG_H

#include <pgmspace.h>

const char index_html[] PROGMEM = R"rawliteral(
{html}
)rawliteral";

#endif
"""

    with open('WebConfig.h', 'w', encoding='utf-8') as f:
        f.write(header_content)
    
    print("Successfully packed WebConfig.h")

if __name__ == "__main__":
    pack_web_config()