/**
 * @file esp32Cam.c
 * @brief Implementación del controlador de la cámara OV2640 parametrizado mediante Kconfig.
 */

#include <stdio.h>
#include "esp32Cam.h"
#include "esp_log.h"
#include "sdkconfig.h" // Archivo autogenerado por ESP-IDF con las opciones de Kconfig

static const char *TAG = "esp32_cam";

// Mapeo de pines utilizando las opciones configurables de Kconfig
#define PWDN_GPIO_NUM     CONFIG_ESP32_CAM_PIN_PWDN
#define RESET_GPIO_NUM    CONFIG_ESP32_CAM_PIN_RESET
#define XCLK_GPIO_NUM     CONFIG_ESP32_CAM_PIN_XCLK
#define SIOD_GPIO_NUM     CONFIG_ESP32_CAM_PIN_SIOD
#define SIOC_GPIO_NUM     CONFIG_ESP32_CAM_PIN_SIOC

#define Y9_GPIO_NUM       CONFIG_ESP32_CAM_PIN_D7
#define Y8_GPIO_NUM       CONFIG_ESP32_CAM_PIN_D6
#define Y7_GPIO_NUM       CONFIG_ESP32_CAM_PIN_D5
#define Y6_GPIO_NUM       CONFIG_ESP32_CAM_PIN_D4
#define Y5_GPIO_NUM       CONFIG_ESP32_CAM_PIN_D3
#define Y4_GPIO_NUM       CONFIG_ESP32_CAM_PIN_D2
#define Y3_GPIO_NUM       CONFIG_ESP32_CAM_PIN_D1
#define Y2_GPIO_NUM       CONFIG_ESP32_CAM_PIN_D0
#define VSYNC_GPIO_NUM    CONFIG_ESP32_CAM_PIN_VSYNC
#define HREF_GPIO_NUM     CONFIG_ESP32_CAM_PIN_HREF
#define PCLK_GPIO_NUM     CONFIG_ESP32_CAM_PIN_PCLK

/**
 * @brief Inicializa el hardware de la cámara OV2640 y asigna sus búferes en PSRAM.
 */
esp_err_t esp_cam_init(void) {
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
    config.pin_sccb_sda = SIOD_GPIO_NUM;
    config.pin_sccb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG; // Formato JPEG para almacenamiento o streaming
    config.frame_size = FRAMESIZE_QVGA;   // Tamaño de imagen configurado
    config.jpeg_quality = 12;             // Calidad de compresión (0 a 63; menor = mayor calidad)
    config.fb_count = 1;
    config.fb_location = CAMERA_FB_IN_PSRAM; // Forzar asignación en PSRAM para evitar desbordamientos
    config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
    config.jpeg_buffer_size = 131072;     // Tamaño del búfer JPEG asignado

    // Inicializar la cámara mediante la librería esp_camera
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Fallo al inicializar la cámara con error 0x%x", err);
        return err;
    }

    ESP_LOGI(TAG, "¡Cámara inicializada correctamente usando Kconfig!");
    return ESP_OK;
}

/**
 * @brief Captura un fotograma (frame buffer) utilizando la cámara.
 */
camera_fb_t* esp_cam_capture(void) {
    return esp_camera_fb_get();
}

/**
 * @brief Libera el búfer de la cámara previamente capturado.
 */
void esp_cam_return(camera_fb_t *fb) {
    esp_camera_fb_return(fb);
}