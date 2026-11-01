#include <stdio.h>
#include "esp32Cam.h"
#include "esp_log.h"

static const char *TAG = "esp32_cam";

#define CAMERA_MODEL_AI_THINKER
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
    config.pixel_format = PIXFORMAT_JPEG; // Formato para streaming o fotos
    config.frame_size = FRAMESIZE_QVGA;   // Tamaño de imagen (QVGA, VGA, etc.)
    config.jpeg_quality = 12;             // De 0 a 63 (menor número = mejor calidad)
    config.fb_count = 1;
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;

//    config.fb_location = CAMERA_FB_IN_PSRAM;      // Forzar asignación en PSRAM para evitar desbordamientos de DMA interno
    config.jpeg_buffer_size = 131072;  // .fb_size = CONFIG_CAMERA_JPEG_MODE_FRAME_SIZE 
  

    // Inicializar la cámara
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Fallo al inicializar la cámara con error 0x%x", err);
        return err;
    }

    ESP_LOGI(TAG, "¡Cámara inicializada correctamente!");
    return ESP_OK;
}

camera_fb_t* esp_cam_capture(void){
    return esp_camera_fb_get();

}
void esp_cam_return(camera_fb_t *fb){
    esp_camera_fb_return(fb);

}
