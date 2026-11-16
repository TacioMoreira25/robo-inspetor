/*
 * =============================================================================
 * CAPTURA DE VÍDEO OTIMIZADA DA ESP32-CAM SLAVE (camera_stream.cpp)
 * =============================================================================
 * OTIMIZAÇÃO DE LARGURA DE BANDA E LATÊNCIA:
 * A captura opera em resolução QVGA (320x240) com compressão JPEG nível 15 e
 * modo `CAMERA_GRAB_LATEST`. Essa configuração limita a taxa de transmissão a
 * ~120 kbps (~4 KB por quadro) e descarta quadros antigos do buffer DMA,
 * garantindo fluidez de imagem sem impactar a latência dos comandos de tração.
 */

#include "camera_stream.h"
#include "config.h"

#if !MODO_SIMULACAO && defined(PLACA_ESP32_CAM)
    #include <Wire.h>
    #include "esp_camera.h"

    // Mapeamento interno oficial dos pinos da câmera OV2640 na placa AI-Thinker ESP32-CAM
    #define PWDN_GPIO_NUM     32
    #define RESET_GPIO_NUM    -1
    #define XCLK_GPIO_NUM      0
    #define SIOD_GPIO_NUM     26
    #define SIOC_GPIO_NUM     27
    #define Y9_GPIO_NUM       35
    #define Y8_GPIO_NUM       34
    #define Y7_GPIO_NUM       39
    #define Y6_GPIO_NUM       36
    #define Y5_GPIO_NUM       21
    #define Y4_GPIO_NUM       19
    #define Y3_GPIO_NUM       18
    #define Y2_GPIO_NUM        5
    #define VSYNC_GPIO_NUM    25
    #define HREF_GPIO_NUM     23
    #define PCLK_GPIO_NUM     22
#endif

// Quadro JPEG mínimo (1x1 pixel) usado apenas quando compilado em modo simulação
static const uint8_t mock_jpeg_frame[] = {
    0xFF, 0xD8, 0xFF, 0xE0, 0x00, 0x10, 0x4A, 0x46, 0x49, 0x46, 0x00, 0x01, 0x01, 0x01, 0x00, 0x48,
    0x00, 0x48, 0x00, 0x00, 0xFF, 0xDB, 0x00, 0x43, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xC0, 0x00, 0x0B, 0x08, 0x00, 0x01, 0x00,
    0x01, 0x01, 0x01, 0x11, 0x00, 0xFF, 0xC4, 0x00, 0x1F, 0x00, 0x00, 0x01, 0x05, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
    0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0xFF, 0xDA, 0x00, 0x08, 0x01, 0x01, 0x00, 0x00, 0x3F, 0x00,
    0xBF, 0x80, 0xFF, 0xD9
};

CameraStream::CameraStream() : _initialized(false), _frame_count(0) {
#if !MODO_SIMULACAO && defined(PLACA_ESP32_CAM)
    _fb_ptr = nullptr;
#endif
}

bool CameraStream::begin() {
#if MODO_SIMULACAO || !defined(PLACA_ESP32_CAM)
    _initialized = true;
    return true;
#else
    // Reinicia fisicamente a alimentação da câmera pelo pino PWDN (GPIO 32) antes do init
    pinMode(PWDN_GPIO_NUM, OUTPUT);
    digitalWrite(PWDN_GPIO_NUM, HIGH);
    delay(100);
    digitalWrite(PWDN_GPIO_NUM, LOW);
    delay(100);

    // Diagnóstico I2C interno da câmera: gera XCLK em GPIO 0 (10 MHz) e sonda o barramento (GPIO 26/27)
    ledcSetup(0, 10000000, 1);
    ledcAttachPin(XCLK_GPIO_NUM, 0);
    ledcWrite(0, 1);
    delay(20);

    Wire.begin(SIOD_GPIO_NUM, SIOC_GPIO_NUM, 100000);
    Wire.beginTransmission(0x30);
    uint8_t i2c_err = Wire.endTransmission();
    Serial.printf("[CAM-DIAG] Probe 0x30 (OV2640): err=%u (%s)\n",
                  i2c_err, i2c_err == 0 ? "LENTE VIVA!" : "SEM RESPOSTA");

    uint8_t found_count = 0;
    for (uint8_t addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            Serial.printf("[CAM-DIAG] Dispositivo I2C encontrado em: 0x%02X\n", addr);
            found_count++;
        }
    }
    if (found_count == 0) {
        Serial.println("[CAM-DIAG] Nenhuma lente/sensor respondeu no I2C interno (GPIO 26/27).");
    }

    // Libera os pinos para o driver oficial esp_camera_init() assumir sem conflito
    Wire.end();
    ledcDetachPin(XCLK_GPIO_NUM);
    delay(20);

    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sscb_sda = SIOD_GPIO_NUM;
    config.pin_sscb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 10000000; // 10 MHz para maior compatibilidade com lotes chineses da OV2640
    config.pixel_format = PIXFORMAT_JPEG;

    // Otimização Anti-Lag: QVGA (320x240) leve (~120 kbps) com descarte de frame atrasado
    config.frame_size = FRAMESIZE_QVGA;
    config.jpeg_quality = 15;
    config.fb_count = psramFound() ? 2 : 1;
    config.grab_mode = CAMERA_GRAB_LATEST;

    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        _initialized = false;
        return false;
    }

    _initialized = true;
    return true;
#endif
}

bool CameraStream::getFrame(const uint8_t **out_buf, size_t *out_len) {
    if (!_initialized) {
        return false;
    }

#if MODO_SIMULACAO || !defined(PLACA_ESP32_CAM)
    *out_buf = mock_jpeg_frame;
    *out_len = sizeof(mock_jpeg_frame);
    _frame_count++;
    return true;
#else
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
        return false;
    }
    _fb_ptr = (void *)fb;
    *out_buf = fb->buf;
    *out_len = fb->len;
    _frame_count++;
    return true;
#endif
}

void CameraStream::releaseFrame() {
#if !MODO_SIMULACAO && defined(PLACA_ESP32_CAM)
    if (_fb_ptr) {
        esp_camera_fb_return((camera_fb_t *)_fb_ptr);
        _fb_ptr = nullptr;
    }
#endif
}
