#include <stdio.h>
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp32Cam.h"
#include "sd.h" // componente SD
#include "wifi_server.h" // componente de red
#include "mqtt_manager.h" // componente MQTT

static const char *TAG = "camera";

//const char* wifi_ssid = "EL_NOMBRE_DE_TU_WIFI";
//const char* wifi_password = "TU_CONTRASEÑA_WIFI";


void app_main(void) {
    
    ESP_LOGI(TAG, "Iniciando sistema...");

    if (esp_cam_init() != ESP_OK) {         // 1. Inicializar la cámara
        ESP_LOGE(TAG, "Error al inicializar la cámara.");
        return;
    }

    if (sd_logger_init() != ESP_OK) {       // 2. Inicializar la Tarjeta SD
        ESP_LOGE(TAG, "No se pudo continuar sin la tarjeta SD.");
        return;
    }
    
    // 2. Levantar red Wi-Fi y Servidor Web
    wifi_init_softap();
    start_webserver();

    // 2. Iniciar el cliente MQTT apuntando a la IP de tu PC (Docker) y puerto 1883
    mqtt_app_start("mqtt://172.24.176.1:1883"); // <-- Reemplaza 192.168.X.X por la IPv4 de tu PC

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

            // 3. Enviar notificación por MQTT mediante la función modular
            char payload[64];
            snprintf(payload, sizeof(payload), "Foto #%d guardada (%u bytes)", pictureNumber, pic->len);
            mqtt_send_status("esp32/camera/status", payload);
            
            esp_cam_return(pic);       //liberar el fotograma para no saturar la memoria PSRAM

        }

        vTaskDelay(pdMS_TO_TICKS(15000));      // Esperar 5 segundos antes de la siguiente foto

    }

}