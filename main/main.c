#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp32Cam.h"
#include "sd.h"
#include "wifi_server.h"
#include "mqtt_manager.h"
#include "espnow_manager.h"
#include "capture_timer.h"


static const char *TAG = "CAMERA_MAIN";

#define BACKUP_CAPTURE_TIME_MS 40000


// MAC STA del NodeMCU
static const uint8_t nodemcu_mac[6] = {
    0xB0, 0x3F, 0xD3, 0x6F, 0x44, 0x44
};


// Indica que el NodeMCU solicito una captura
static volatile bool capture_requested = false;


// Indica que el NodeMCU confirmo FOTO_OK
static volatile bool confirmation_received = false;


/*
 * Esta funcion se ejecuta cuando llega un mensaje ESP-NOW.
 *
 * No captura la foto directamente porque el callback de ESP-NOW
 * debe mantenerse corto. Solo modifica banderas que luego
 * procesa el bucle principal.
 */
static void process_received_message(
    const uint8_t *source_mac,
    const uint8_t *data,
    int data_len)
{
    if (source_mac == NULL || data == NULL || data_len <= 0)
    {
        ESP_LOGW(TAG, "Mensaje ESP-NOW invalido");
        return;
    }


    char message[250];

    int copy_length = data_len;

    if (copy_length >= sizeof(message))
    {
        copy_length = sizeof(message) - 1;
    }


    memcpy(message, data, copy_length);

    message[copy_length] = '\0';


    ESP_LOGI(
        TAG,
        "Mensaje recibido de %02X:%02X:%02X:%02X:%02X:%02X: %s",
        source_mac[0],
        source_mac[1],
        source_mac[2],
        source_mac[3],
        source_mac[4],
        source_mac[5],
        message
    );


    if (strcmp(message, "CAPTURAR") == 0)
    {
        capture_requested = true;

        /*
         * Reiniciar el temporizador porque la comunicacion
         * con el NodeMCU sigue activa.
         */
        capture_timer_reset();

        ESP_LOGI(TAG, "Orden CAPTURAR registrada");
    }
    else if (strcmp(message, "OK") == 0)
    {
        confirmation_received = true;

        ESP_LOGI(TAG, "Confirmacion OK registrada");
    }
    else
    {
        ESP_LOGW(TAG, "Mensaje desconocido: %s", message);
    }
}


/*
 * Envia un mensaje al NodeMCU.
 */
static esp_err_t send_message_to_nodemcu(const char *message)
{
    if (message == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    ESP_LOGI(TAG, "Enviando al NodeMCU: %s", message);

    return espnow_manager_send( nodemcu_mac, message, strlen(message));

}


/*
 * Captura una foto y la guarda en la tarjeta SD.
 */
static bool capture_and_save_photo(int picture_number)
{
    ESP_LOGI(TAG, "Capturando foto numero %d", picture_number);

    camera_fb_t *picture = esp_cam_capture();

    if (picture == NULL)
    {
        ESP_LOGE(TAG, "No se pudo capturar la foto");
        return false;
    }


    /*
     * funcion para guardar la imagen.
     */
    sd_logger_save_image(picture->buf, picture->len, picture_number);
    ESP_LOGI(TAG, "Foto guardada correctamente. Tamano: %u bytes", picture->len);

    /*
     * Mantener la notificacion MQTT.
     */
    char mqtt_payload[64];

    snprintf(
        mqtt_payload,
        sizeof(mqtt_payload),
        "Foto #%d guardada (%u bytes)",
        picture_number,
        picture->len
    );


    mqtt_send_status(
        "esp32/camera/status",
        mqtt_payload
    );


    /*
     * Liberar el frame de la camara.
     */
    esp_cam_return(picture);


    /*
     * Avisar al NodeMCU.
     *
     * Esto se envia tanto para una foto solicitada
     * como para una foto automatica de respaldo.
     */
    esp_err_t result = send_message_to_nodemcu("FOTO_OK");

    if (result != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "No se pudo enviar FOTO_OK: %s",
            esp_err_to_name(result)
        );
    }
    else
    {
        ESP_LOGI(TAG, "FOTO_OK enviado");
    }


    return true;
}


void app_main(void)
{
    ESP_LOGI(TAG, "Iniciando ESP32-CAM");


    // 1. Inicializar camara
    if (esp_cam_init() != ESP_OK)
    {
        ESP_LOGE(TAG, "Error inicializando la camara");
        return;
    }


    // 2. Inicializar tarjeta SD
    if (sd_logger_init() != ESP_OK)
    {
        ESP_LOGE(TAG, "Error inicializando la tarjeta SD");
        return;
    }


    /*
     * 3. Inicializar Wi-Fi.
     */
    wifi_init_softap();


    // 4. Iniciar servidor web
    start_webserver();


    /*
     * 5. Inicializar ESP-NOW.
     * Debe ejecutarse despues de wifi_init_softap().
     */
    if (espnow_manager_init() != ESP_OK)
    {
        ESP_LOGE(TAG, "No se pudo inicializar ESP-NOW");
        return;
    }


    // 6. Registrar callback de recepcion
    if (espnow_manager_register_receive_callback(
            process_received_message) != ESP_OK)
    {
        ESP_LOGE(TAG, "No se pudo registrar el callback ESP-NOW");
        return;
    }

    // 7. Agregar NodeMCU como peer
    if (espnow_manager_add_peer(nodemcu_mac, 0) != ESP_OK)
    {
        ESP_LOGE(TAG, "No se pudo agregar el NodeMCU");
        return;
    }


    /*
     * 8. Mantener MQTT.
     *
     * Si el broker no esta disponible, ESP-NOW puede
     * continuar funcionando independientemente.
     */
    mqtt_app_start("mqtt://172.24.176.1:1883");


    /*
     * 9. Temporizador de respaldo.
     * Si durante 40 segundos no se recibe CAPTURAR,
     * la camara toma una foto automaticamente.
     */
    capture_timer_init(BACKUP_CAPTURE_TIME_MS);


    ESP_LOGI(TAG, "ESP32-CAM lista");
    ESP_LOGI(TAG, "Esperando orden CAPTURAR");
    ESP_LOGI(TAG, "Captura automatica configurada cada 40 segundos");

    int picture_number = 0;

    while (1)
    {
        bool automatic_capture = capture_timer_expired();

        /*
         * Se captura si:
         * 1. El NodeMCU envio CAPTURAR.
         * 2. Vencio el temporizador de 40 segundos.
         */
        if (capture_requested || automatic_capture)
        {
            if (capture_requested)
            {
                ESP_LOGI(TAG, "Iniciando captura solicitada por NodeMCU");
            }
            else
            {
                ESP_LOGW(
                    TAG,
                    "Temporizador vencido. "
                    "Iniciando captura automatica"
                );
            }


            /*
             * Limpiar la solicitud antes de capturar.
             */
            capture_requested = false;


            /*
             * Reiniciar el contador de respaldo.
             *
             * La proxima captura automatica sera
             * dentro de otros 40 segundos.
             */
            capture_timer_reset();


            picture_number++;


            bool capture_success =
                capture_and_save_photo(picture_number);


            if (!capture_success)
            {
                ESP_LOGE(
                    TAG,
                    "Fallo la captura numero %d",
                    picture_number
                );
            }
        }


        /*
         * Registrar la confirmacion recibida.
         */
        if (confirmation_received)
        {
            confirmation_received = false;

            ESP_LOGI(
                TAG,
                "NodeMCU confirmo la recepcion de FOTO_OK"
            );
        }


        vTaskDelay(pdMS_TO_TICKS(100));
    }
}