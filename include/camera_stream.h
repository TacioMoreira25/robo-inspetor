#pragma once

#include <Arduino.h>

class CameraStream {
public:
    CameraStream();

    // Inicialização do hardware da câmera ou Mock da simulação
    bool begin();

    // Verificação de prontidão do streaming
    bool isReady() const { return _initialized; }

    // Obtenção de quadro JPEG (emulação de buffer para mock ou captura DMA na câmera física)
    bool getFrame(const uint8_t **out_buf, size_t *out_len);
    void releaseFrame();

private:
    bool _initialized;
    uint32_t _frame_count;

#if !MODO_SIMULACAO && defined(PLACA_ESP32_CAM)
    void *_fb_ptr;
#endif
};
