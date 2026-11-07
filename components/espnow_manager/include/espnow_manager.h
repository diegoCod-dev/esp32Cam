#ifndef ESPNOW_MANAGER_H
#define ESPNOW_MANAGER_H

#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

/**
 * Callback definido por la aplicacion.
 *
 * Se ejecuta automaticamente cuando llega un mensaje ESP-NOW.
 *
 * source_mac: MAC del dispositivo que envio el mensaje.
 * data: datos recibidos.
 * data_len: cantidad de bytes recibidos.
 */
typedef void (*espnow_receive_callback_t)(
    const uint8_t *source_mac,
    const uint8_t *data,
    int data_len
);

/**
 * Inicializa ESP-NOW.
 *
 * IMPORTANTE:
 * Wi-Fi debe haber sido inicializado previamente.
 */
esp_err_t espnow_manager_init(void);

/**
 * Registra la funcion de la aplicacion que procesara
 * los mensajes recibidos.
 */
esp_err_t espnow_manager_register_receive_callback(
    espnow_receive_callback_t callback
);

/**
 * Agrega un dispositivo remoto mediante su MAC.
 *
 * channel = 0 utiliza el canal Wi-Fi actual.
 */
esp_err_t espnow_manager_add_peer(
    const uint8_t *peer_mac,
    uint8_t channel
);

/**
 * Envia datos a un dispositivo previamente agregado.
 */
esp_err_t espnow_manager_send(
    const uint8_t *destination_mac,
    const void *data,
    size_t data_len
);

#endif