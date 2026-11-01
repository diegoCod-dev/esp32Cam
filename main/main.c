#include <stdio.h>
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp32Cam.h"


static const char *TAG = "camera_demo";

void app_main(void) {
    

    // Inicializar la cámara
    ESP_LOGI(TAG, "Iniciando sistema...");

    if (esp_cam_init() != ESP_OK) {
        ESP_LOGE(TAG, "Error al inicializar la cámara.");
        return;
    }

    ESP_LOGI(TAG, "Cámara inicializada como componente con éxito.");
    vTaskDelay(pdMS_TO_TICKS(2000));    // WAIT AL SENSOR ANTES DE CAPTURAR LA PRIMERA FOTO

    // Bucle para probar la captura de fotos
    while (1) {
        // Capturar un fotograma
        camera_fb_t *pic = esp_cam_capture();
        if (!pic) {
            ESP_LOGE(TAG, "Fallo al capturar la foto");
        } else {
            ESP_LOGI(TAG, "¡Foto tomada con éxito! Tamaño: %u bytes", pic->len);
            
            //liberar el fotograma para no saturar la memoria PSRAM
            esp_cam_return(pic);
        }

        // Esperar 5 segundos antes de la siguiente foto
        vTaskDelay(pdMS_TO_TICKS(5000));
    }

}