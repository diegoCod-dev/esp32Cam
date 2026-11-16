/**
 * @file espnow_manager.h
 * @brief Gestor y abstracción del protocolo ESP-NOW para ESP-IDF.
 * 
 * Este módulo simplifica la inicialización de ESP-NOW, el registro de callbacks 
 * personalizados para la recepción de mensajes, la gestión de dispositivos remotos (peers) 
 * y el envío de datos de forma directa entre dispositivos ESP32 sin necesidad de un router.
 */

#ifndef ESPNOW_MANAGER_H
#define ESPNOW_MANAGER_H

#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

/**
 * @brief Tipo de función de retrollamada (callback) para la recepción de datos.
 * 
 * Se ejecuta automáticamente cuando llega un mensaje ESP-NOW desde un dispositivo remoto.
 * 
 * @param source_mac Puntero a la dirección MAC de 6 bytes del dispositivo que envió el mensaje.
 * @param data Puntero a los datos crudos recibidos.
 * @param data_len Cantidad de bytes recibidos en el mensaje.
 */
typedef void (*espnow_receive_callback_t)(
    const uint8_t *source_mac,
    const uint8_t *data,
    int data_len
);

/**
 * @brief Inicializa el subsistema ESP-NOW.
 * 
 * Configura el protocolo y registra los callbacks internos por defecto para la recepción 
 * y el estado de envío de los mensajes.
 * 
 * @note IMPORTANTE: El stack de Wi-Fi debe haber sido inicializado previamente.
 * 
 * @return esp_err_t 
 *         - ESP_OK si la inicialización fue exitosa.
 *         - Otro código de error de ESP-IDF en caso de fallo.
 */
esp_err_t espnow_manager_init(void);

/**
 * @brief Registra la función de la aplicación que procesará los mensajes recibidos.
 * 
 * Permite desacoplar el componente de red de la lógica de negocio de la aplicación.
 * 
 * @param callback Puntero a la función de tipo espnow_receive_callback_t.
 * @return esp_err_t 
 *         - ESP_OK si se registró correctamente.
 *         - ESP_ERR_INVALID_ARG si el callback proporcionado es NULL.
 */
esp_err_t espnow_manager_register_receive_callback(
    espnow_receive_callback_t callback
);

/**
 * @brief Agrega un dispositivo remoto (peer) mediante su dirección MAC.
 * 
 * Registra un nodo en la tabla de pares de ESP-NOW para permitir la comunicación bidireccional.
 * 
 * @param peer_mac Puntero a la dirección MAC de 6 bytes del dispositivo remoto.
 * @param channel Canal Wi-Fi a utilizar (si se pasa 0, utiliza el canal Wi-Fi actual).
 * @return esp_err_t 
 *         - ESP_OK si el dispositivo fue agregado con éxito o ya existía.
 *         - ESP_ERR_INVALID_ARG si la MAC es NULL.
 *         - Otro código de error de ESP-IDF si falla el registro.
 */
esp_err_t espnow_manager_add_peer(
    const uint8_t *peer_mac,
    uint8_t channel
);

/**
 * @brief Envía datos a un dispositivo remoto previamente agregado.
 * 
 * @param destination_mac Puntero a la dirección MAC de destino (6 bytes).
 * @param data Puntero a los datos que se desean enviar.
 * @param data_len Longitud en bytes de los datos a transmitir.
 * @return esp_err_t 
 *         - ESP_OK si el paquete se encoló correctamente para el envío.
 *         - ESP_ERR_INVALID_ARG si los argumentos son inválidos o nulos.
 *         - ESP_ERR_NOT_FOUND si el dispositivo destino no ha sido agregado previamente.
 *         - Otro código de error devuelto por esp_now_send().
 */
esp_err_t espnow_manager_send(
    const uint8_t *destination_mac,
    const void *data,
    size_t data_len
);

#endif // ESPNOW_MANAGER_H