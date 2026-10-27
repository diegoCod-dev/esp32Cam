#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_cam.h"

static const char *TAG = "main";

void app_main(void) {
    ESP_LOGI(TAG, "Iniciando sistema...");

    if (esp_cam_init() != ESP_OK) {
        ESP_LOGE(TAG, "Error al inicializar la cámara.");
        return;
    }

    ESP_LOGI(TAG, "Cámara inicializada como componente con éxito.");

    vTaskDelay(pdMS_TO_TICKS(2000));    // WAIT AL SENSOR ANTES DE CAPTURAR LA PRIMERA FOTO

    while (1) {
        camera_fb_t *pic = esp_cam_capture();
        if (!pic) {
            ESP_LOGE(TAG, "Fallo al capturar la foto");
        } else {
            ESP_LOGI(TAG, "¡Foto tomada con éxito! Tamaño: %u bytes", pic->len);
            esp_cam_return(pic);
        }

        vTaskDelay(pdMS_TO_TICKS(10000)); // <--- 10 segundos de pausa


    }
}