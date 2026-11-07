#include "espnow_manager.h"

#include <string.h>

#include "esp_log.h"
#include "esp_now.h"
#include "esp_wifi.h"

static const char *TAG = "ESPNOW_MANAGER";

static espnow_receive_callback_t application_receive_callback = NULL;


/*
 * Callback interno de ESP-IDF.
 * ESP-IDF lo ejecuta automaticamente cuando llega un mensaje.
 */
static void espnow_internal_receive_callback(
    const esp_now_recv_info_t *receive_info,
    const uint8_t *data,
    int data_len
)
{
    if (receive_info == NULL || data == NULL || data_len <= 0) {
        ESP_LOGW(TAG, "Mensaje recibido invalido");
        return;
    }

    ESP_LOGI(
        TAG,
        "Mensaje recibido desde %02X:%02X:%02X:%02X:%02X:%02X",
        receive_info->src_addr[0],
        receive_info->src_addr[1],
        receive_info->src_addr[2],
        receive_info->src_addr[3],
        receive_info->src_addr[4],
        receive_info->src_addr[5]
    );

    /*
     * Entregar el mensaje a la aplicacion.
     * El componente no decide que significa el mensaje.
     */
    if (application_receive_callback != NULL) {
        application_receive_callback(
            receive_info->src_addr,
            data,
            data_len
        );
    }
}


/*
 * Callback interno ejecutado cuando finaliza
 * un intento de envio.
 */
static void espnow_internal_send_callback(
    const wifi_tx_info_t *tx_info,
    esp_now_send_status_t status
)
{
    /*
     * Evita una advertencia del compilador porque por ahora
     * no utilizamos la informacion adicional del envio.
     */
    (void)tx_info;

    if (status == ESP_NOW_SEND_SUCCESS) {
        ESP_LOGI(TAG, "Mensaje entregado correctamente");
    } else {
        ESP_LOGW(TAG, "No se pudo entregar el mensaje");
    }
}


esp_err_t espnow_manager_init(void)
{
    ESP_LOGI(TAG, "Inicializando ESP-NOW");

    /*
     * Este componente asume que Wi-Fi ya fue inicializado.
     *
     * Temporalmente se inicializa desde main.c.
     * Mas adelante se utilizara un componente Wi-Fi reusable.
     */

    esp_err_t result = esp_now_init();

    if (result != ESP_OK) {
        ESP_LOGE(
            TAG,
            "Error inicializando ESP-NOW: %s",
            esp_err_to_name(result)
        );

        return result;
    }

    result = esp_now_register_recv_cb(
        espnow_internal_receive_callback
    );

    if (result != ESP_OK) {
        ESP_LOGE(
            TAG,
            "Error registrando callback de recepcion: %s",
            esp_err_to_name(result)
        );

        return result;
    }

    result = esp_now_register_send_cb(
        espnow_internal_send_callback
    );

    if (result != ESP_OK) {
        ESP_LOGE(
            TAG,
            "Error registrando callback de envio: %s",
            esp_err_to_name(result)
        );

        return result;
    }

    ESP_LOGI(TAG, "ESP-NOW inicializado correctamente");

    return ESP_OK;
}


esp_err_t espnow_manager_register_receive_callback(
    espnow_receive_callback_t callback
)
{
    if (callback == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    application_receive_callback = callback;

    ESP_LOGI(TAG, "Callback de recepcion registrado");

    return ESP_OK;
}


esp_err_t espnow_manager_add_peer(
    const uint8_t *peer_mac,
    uint8_t channel
)
{
    if (peer_mac == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (esp_now_is_peer_exist(peer_mac)) {
        ESP_LOGI(TAG, "El dispositivo ya estaba registrado");
        return ESP_OK;
    }

    esp_now_peer_info_t peer_information = {0};

    memcpy(
        peer_information.peer_addr,
        peer_mac,
        ESP_NOW_ETH_ALEN
    );

    peer_information.channel = channel;
    peer_information.ifidx = WIFI_IF_STA;
    peer_information.encrypt = false;

    esp_err_t result = esp_now_add_peer(&peer_information);

    if (result != ESP_OK) {
        ESP_LOGE(
            TAG,
            "Error agregando dispositivo: %s",
            esp_err_to_name(result)
        );

        return result;
    }

    ESP_LOGI(
        TAG,
        "Dispositivo agregado: %02X:%02X:%02X:%02X:%02X:%02X",
        peer_mac[0],
        peer_mac[1],
        peer_mac[2],
        peer_mac[3],
        peer_mac[4],
        peer_mac[5]
    );

    return ESP_OK;
}


esp_err_t espnow_manager_send(
    const uint8_t *destination_mac,
    const void *data,
    size_t data_len
)
{
    if (destination_mac == NULL ||
        data == NULL ||
        data_len == 0) {

        return ESP_ERR_INVALID_ARG;
    }

    if (!esp_now_is_peer_exist(destination_mac)) {
        ESP_LOGE(TAG, "El dispositivo destino no fue agregado");
        return ESP_ERR_NOT_FOUND;
    }

    return esp_now_send(
        destination_mac,
        (const uint8_t *)data,
        data_len
    );
}