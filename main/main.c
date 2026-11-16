/**
 * @file main.c
 * @brief Archivo principal de control para el sistema ESP32-CAM.
 * 
 * Este programa unifica la captura de imágenes mediante la cámara, el almacenamiento local 
 * en tarjeta SD, la conectividad híbrida Wi-Fi (APSTA), la comunicación de red local de 
 * baja latencia mediante ESP-NOW con un NodeMCU, el reporte de telemetría por MQTT y 
 * un temporizador de respaldo para capturas periódicas automáticas.
 */

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

/** @brief Etiqueta (TAG) para los logs del módulo principal. */
static const char *TAG = "CAMERA_MAIN";

/** @brief Tiempo de respaldo en milisegundos (40 segundos) para disparar una captura automática. */
#define BACKUP_CAPTURE_TIME_MS 40000

/** @brief Dirección MAC de la estación (STA) del NodeMCU asociado. */
static const uint8_t nodemcu_mac[6] = {
    0xB0, 0x3F, 0xD3, 0x6F, 0x44, 0x44
};

/** @brief Bandera volátil que indica si el NodeMCU solicitó una captura de manera remota. */
static volatile bool capture_requested = false;

/** @brief Bandera volátil que indica la confirmación de recepción FOTO_OK por parte del NodeMCU. */
static volatile bool confirmation_received = false;

/**
 * @brief Función de retrollamada (callback) para procesar los mensajes recibidos por ESP-NOW.
 * 
 * Se ejecuta automáticamente al llegar un paquete. Se mantiene ligera modificando banderas 
 * que luego procesará el bucle principal de la aplicación.
 * 
 * @param source_mac Puntero a la dirección MAC de 6 bytes del emisor.
 * @param data Puntero a los datos recibidos.
 * @param data_len Longitud en bytes de los datos recibidos.
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

/**
 * @brief Envía un mensaje de texto plano hacia el NodeMCU mediante ESP-NOW.
 * 
 * @param message Puntero a la cadena de caracteres a transmitir.
 * @return esp_err_t ESP_OK si se encoló con éxito, o código de error en caso contrario.
 */
static esp_err_t send_message_to_nodemcu(const char *message)
{
    if (message == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    ESP_LOGI(TAG, "Enviando al NodeMCU: %s", message);
    return espnow_manager_send(nodemcu_mac, message, strlen(message));
}

/**
 * @brief Captura una fotografía, la almacena en la tarjeta SD, notifica por MQTT y avisa al NodeMCU.
 * 
 * @param picture_number Número secuencial identificador de la foto actual.
 * @return true Si el proceso de captura, guardado y notificación fue exitoso.
 * @return false Si ocurrió un error crítico en la cámara.
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

    // Guardar la imagen utilizando el componente SD reutilizable
    sd_logger_save_image(picture->buf, picture->len, picture_number);
    ESP_LOGI(TAG, "Foto guardada correctamente. Tamano: %u bytes", picture->len);

    // Enviar notificación de estado vía MQTT
    char mqtt_payload[64];
    snprintf(
        mqtt_payload,
        sizeof(mqtt_payload),
        "Foto #%d guardada (%u bytes)",
        picture_number,
        picture->len
    );

    mqtt_send_status("esp32/camera/status", mqtt_payload);

    // Liberar el búfer (frame buffer) de la cámara
    esp_cam_return(picture);

    // Notificar al NodeMCU que la foto está lista
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

/**
 * @brief Punto de entrada principal de la aplicación ESP-IDF.
 */
void app_main(void)
{
    ESP_LOGI(TAG, "Iniciando ESP32-CAM");

    // 1. Inicializar cámara
    if (esp_cam_init() != ESP_OK)
    {
        ESP_LOGE(TAG, "Error inicializando la camara");
        return;
    }

    // 2. Inicializar tarjeta SD usando el componente reutilizable (configuración por defecto con NULL)
    if (sd_logger_init(NULL) != ESP_OK)
    {
        ESP_LOGE(TAG, "Error inicializando la tarjeta SD");
        return;
    }

    // 3. Inicializar red Wi-Fi en modo híbrido APSTA
    wifi_init_softap();

    // 4. Iniciar servidor web local
    start_webserver();

    // 5. Inicializar ESP-NOW (debe ejecutarse después de inicializar el Wi-Fi)
    if (espnow_manager_init() != ESP_OK)
    {
        ESP_LOGE(TAG, "No se pudo inicializar ESP-NOW");
        return;
    }

    // 6. Registrar callback de recepción de ESP-NOW
    if (espnow_manager_register_receive_callback(process_received_message) != ESP_OK)
    {
        ESP_LOGE(TAG, "No se pudo registrar el callback ESP-NOW");
        return;
    }

    // 7. Agregar al NodeMCU como nodo peer registrado
    if (espnow_manager_add_peer(nodemcu_mac, 0) != ESP_OK)
    {
        ESP_LOGE(TAG, "No se pudo agregar el NodeMCU");
        return;
    }

    // 8. Iniciar cliente MQTT (hacia el broker en Docker)
    mqtt_app_start("mqtt://172.24.176.1:1883");

    // 9. Inicializar el temporizador de respaldo (backup)
    capture_timer_init(BACKUP_CAPTURE_TIME_MS);

    ESP_LOGI(TAG, "ESP32-CAM lista");
    ESP_LOGI(TAG, "Esperando orden CAPTURAR");
    ESP_LOGI(TAG, "Captura automatica configurada cada 40 segundos");

    int picture_number = 0;

    // Bucle principal del sistema
    while (1)
    {
        bool automatic_capture = capture_timer_expired();

        /*
         * Se ejecuta la captura si:
         * 1. El NodeMCU solicitó una captura mediante comando ESP-NOW.
         * 2. Venció el temporizador de respaldo de 40 segundos.
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

            // Limpiar la bandera de solicitud antes de proceder
            capture_requested = false;

            // Reiniciar el contador de respaldo
            capture_timer_reset();

            picture_number++;

            bool capture_success = capture_and_save_photo(picture_number);

            if (!capture_success)
            {
                ESP_LOGE(
                    TAG,
                    "Fallo la captura numero %d",
                    picture_number
                );
            }
        }

        // Registrar confirmación recibida del NodeMCU
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