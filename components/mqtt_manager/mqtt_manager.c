#include <stdio.h>
#include "esp_log.h"
#include "mqtt_client.h"
#include "mqtt_manager.h"

static const char *TAG = "MQTT_MANAGER";
static esp_mqtt_client_handle_t client = NULL;

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = event_data;
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "¡Conectado exitosamente al broker MQTT local!");
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "Desconectado del broker MQTT");
            break;
        default:
            break;
    }
}

void mqtt_app_start(const char *broker_uri) {
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = broker_uri,
    };
    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}

void mqtt_send_status(const char *topic, const char *message) {
    if (client != NULL) {
        esp_mqtt_client_publish(client, topic, message, 0, 1, 0);
    } else {
        ESP_LOGW(TAG, "Cliente MQTT no inicializado, no se pudo publicar.");
    }
}