/*
 * =============================================================================
 * ROBÔ INSPETOR TERRESTRE (UGV) - ESP32-CAM (SLAVE DEDICADA AO STREAMING)
 * Arquivo: esp32_cam_stream.ino
 * =============================================================================
 * Responsabilidades:
 * - Isolada de fios de controle nas GPIOs 12, 13, 14 e 15 (boot limpo sem travamentos)
 * - Conecta automaticamente no Wi-Fi "ROBO_INSPETOR_UGV" criado pela ESP32 DevKit V1
 * - Configura IP estático 192.168.4.2 e aplica WiFi.setSleep(false) + WIFI_POWER_15dBm
 * - Inicializa câmera OV2640 em QVGA leve (320x240) com CAMERA_GRAB_LATEST (zero atraso)
 * - Disponibiliza servidor MJPEG contínuo na porta 81 (http://192.168.4.2:81/stream)
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "esp_camera.h"

// =============================================================================
// 1. PINOS INTERNOS DO MÓDULO CAMERA_MODEL_AI_THINKER (OV2640)
// =============================================================================
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

// =============================================================================
// 2. CONEXÃO NA REDE PRÓPRIA DA MASTER ("ROBO_INSPETOR_UGV" -> IP 192.168.4.2)
// =============================================================================
const char* WIFI_SSID = "ROBO_INSPETOR_UGV";
const char* WIFI_PASS = "12345678";

IPAddress local_IP(192, 168, 4, 2);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);

WebServer streamServer(81);

void handleMjpegStream() {
    WiFiClient client = streamServer.client();
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: multipart/x-mixed-replace; boundary=frame");
    client.println("Access-Control-Allow-Origin: *");
    client.println();

    while (client.connected()) {
        camera_fb_t *fb = esp_camera_fb_get();
        if (!fb) {
            delay(10);
            continue;
        }
        client.println("--frame");
        client.println("Content-Type: image/jpeg");
        client.println("Content-Length: " + String(fb->len));
        client.println();
        client.write(fb->buf, fb->len);
        client.println();
        esp_camera_fb_return(fb);
        delay(30);
    }
}

void setup() {
    Serial.begin(115200);

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
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;

    // Otimização Anti-Lag: QVGA leve (320x240) + descarte de quadros atrasados
    config.frame_size = FRAMESIZE_QVGA;
    config.jpeg_quality = 15;
    config.fb_count = psramFound() ? 2 : 1;
    config.grab_mode = CAMERA_GRAB_LATEST;

    esp_camera_init(&config);

    WiFi.mode(WIFI_STA);
    WiFi.config(local_IP, gateway, subnet);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    // Otimizações Anti-Lag e Anti-Ruído RF no chassi
    WiFi.setSleep(false);
    WiFi.setTxPower(WIFI_POWER_15dBm);

    while (WiFi.status() != WL_CONNECTED) {
        delay(100);
    }

    streamServer.on("/stream", HTTP_GET, handleMjpegStream);
    streamServer.begin();
}

void loop() {
    streamServer.handleClient();
}
