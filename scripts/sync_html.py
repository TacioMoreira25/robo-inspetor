# Script de pré-compilação do PlatformIO:
# Une `web/index.html` + `web/style.css` + `web/app.js` em uma única página e
# grava o resultado em `include/web_dashboard.h` (PROGMEM) antes da compilação.
#
# Motivo: no desenvolvimento os arquivos ficam separados (legibilidade e
# visualização direta no navegador), mas o ESP32 entrega a página inteira em
# uma única requisição HTTP (rota "/"), sem precisar de rotas extras para
# CSS/JS nem de sistema de arquivos (SPIFFS/LittleFS).

import os

Import("env")

project_dir = env.subst("$PROJECT_DIR")
web_dir = os.path.join(project_dir, "web")
html_path = os.path.join(web_dir, "index.html")
css_path = os.path.join(web_dir, "style.css")
js_path = os.path.join(web_dir, "app.js")
header_path = os.path.join(project_dir, "include", "web_dashboard.h")


def read(path):
    with open(path, "r", encoding="utf-8") as f:
        return f.read()


if os.path.exists(html_path):
    html_content = read(html_path)

    # Inlining: substitui as referências externas pelo conteúdo dos arquivos
    if os.path.exists(css_path):
        html_content = html_content.replace(
            '<link rel="stylesheet" href="style.css">',
            "<style>\n" + read(css_path) + "</style>",
        )
    if os.path.exists(js_path):
        html_content = html_content.replace(
            '<script src="app.js"></script>',
            "<script>\n" + read(js_path) + "</script>",
        )

    header_content = (
        "#pragma once\n\n"
        "/*\n"
        " * =============================================================================\n"
        " * EMPACOTADOR DO PAINEL WEB EM MEMÓRIA FLASH (web_dashboard.h)\n"
        " * =============================================================================\n"
        " * ARQUIVO GERADO AUTOMATICAMENTE - NÃO EDITAR MANUALMENTE.\n"
        " * Origem: web/index.html + web/style.css + web/app.js, unidos pelo script\n"
        " * `scripts/sync_html.py`. Armazenado em memória Flash (PROGMEM),\n"
        " * preservando a memória RAM dinâmica do microcontrolador.\n"
        " */\n\n"
        "#include <Arduino.h>\n\n"
        'static const char HTML_DASHBOARD[] PROGMEM = R"rawliteral(\n'
        + html_content
        + '\n)rawliteral";\n'
    )

    with open(header_path, "w", encoding="utf-8") as f:
        f.write(header_content)
