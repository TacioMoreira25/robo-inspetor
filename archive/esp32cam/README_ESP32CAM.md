# ESP32-CAM Slave — Código Arquivado

O módulo da ESP32-CAM (`include/camera_stream.h` e `src/camera_stream.cpp`) foi desativado do fluxo principal após o diagnóstico físico de falha na lente OV2640 (`ESP_ERR_NOT_FOUND / 0x105`).

O vídeo ao vivo do robô agora é fornecido por um **celular Android com o app IP Webcam** conectado diretamente à rede Wi-Fi `ROBO_INSPETOR_UGV`, com URL configurável diretamente no Dashboard Web (`http://192.168.4.1`).

Caso uma nova lente OV2640 ou ESP32-CAM seja instalada no futuro:
1. Descomente `[env:esp32cam_hardware]` no `platformio.ini`.
2. Restaure a chamada `beginSlaveCam`/`updateSlaveCam` em `src/main.cpp` e `src/telemetry.cpp`.
3. No campo de vídeo do Dashboard Web, informe `192.168.4.50:81/stream`.
