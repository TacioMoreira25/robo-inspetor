#pragma once

/*
 * =============================================================================
 * MÓDULO DE CAPTURA DE VÍDEO DA ESP32-CAM SLAVE (camera_stream.h)
 * =============================================================================
 * PROPÓSITO E ABSTRAÇÃO DE HARDWARE:
 * Na compilação para a ESP32-CAM (`env:esp32cam_hardware`), inicializa o sensor
 * óptico OV2640 via barramento paralelo DVP/DMA para captura de quadros JPEG.
 * Na compilação para a ESP32 DevKit V1 ou ambiente de simulação, provê um buffer
 * estático leve para manter a compatibilidade de compilação entre plataformas.
 */

#include <Arduino.h>

class CameraStream {
public:
    CameraStream();

    // Configura os pinos internos da câmera OV2640 (na ESP32-CAM) em modo QVGA leve
    bool begin();

    // Informa se a câmera iniciou sem erros
    bool isReady() const { return _initialized; }

    // Captura um quadro JPEG da memória da câmera para enviar via Wi-Fi
    bool getFrame(const uint8_t **out_buf, size_t *out_len);

    // Devolve a memória do quadro após o envio para não travar a RAM da ESP32-CAM
    void releaseFrame();

private:
    bool _initialized;
    uint32_t _frame_count;

#if !MODO_SIMULACAO && defined(PLACA_ESP32_CAM)
    void *_fb_ptr;
#endif
};
