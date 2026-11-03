#include <stdio.h>
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp32Cam.h"
#include "sd.h" // Incluyes tu nuevo componente

static const char *TAG = "camera";

const char* wifi_ssid = "EL_NOMBRE_DE_TU_WIFI";
const char* wifi_password = "TU_CONTRASEÑA_WIFI";


void app_main(void) {
    
    ESP_LOGI(TAG, "Iniciando sistema...");

    if (esp_cam_init() != ESP_OK) {         // Inicializar la cámara
        ESP_LOGE(TAG, "Error al inicializar la cámara.");
        return;
    }

    // 2. Inicializar la Tarjeta SD usando tu función modular
    if (sd_logger_init() != ESP_OK) {
        ESP_LOGE(TAG, "No se pudo continuar sin la tarjeta SD.");
        return;
    }
    

    ESP_LOGI(TAG, "Sistema listo. Iniciando bucle de captura...");
    vTaskDelay(pdMS_TO_TICKS(2000));    // WAIT AL SENSOR ANTES DE CAPTURAR LA PRIMERA FOTO

    int pictureNumber = 0;

    while (1) {
        camera_fb_t *pic = esp_cam_capture();         // Capturar un fotograma

        if (!pic) {
            ESP_LOGE(TAG, "Fallo al capturar la foto");
        } else {
            pictureNumber++;
            sd_logger_save_image(pic->buf, pic->len, pictureNumber);      // Guardar usando la función del componente

            ESP_LOGI(TAG, "¡ya guarde en la sd! Tamaño: %u bytes", pic->len); 
            esp_cam_return(pic);       //liberar el fotograma para no saturar la memoria PSRAM

        }

        vTaskDelay(pdMS_TO_TICKS(5000));      // Esperar 5 segundos antes de la siguiente foto

    }

}