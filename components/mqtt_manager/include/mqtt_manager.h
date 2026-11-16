/**
 * @file mqtt_manager.h
 * @brief Gestor de cliente MQTT para la publicación de telemetría y estado.
 * 
 * Este módulo simplifica la inicialización del cliente MQTT en ESP-IDF 
 * y proporciona funciones para conectarse a un broker local y enviar mensajes de estado.
 */

#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <stdint.h>

/**
 * @brief Inicia y configura el cliente MQTT del sistema.
 * 
 * Establece la conexión con el broker indicado mediante la URI proporcionada 
 * y registra los eventos de conexión y desconexión.
 * 
 * @param broker_uri Cadena de caracteres con la URI del broker MQTT (ej. "mqtt://192.168.0.21:1883").
 */
void mqtt_app_start(const char *broker_uri);

/**
 * @brief Publica un mensaje de estado en un tópico MQTT específico.
 * 
 * @param topic Tópico MQTT donde se publicará el mensaje (ej. "esp32/camera/status").
 * @param message Cadena de texto con el contenido o payload del mensaje a enviar.
 */
void mqtt_send_status(const char *topic, const char *message);

#endif // MQTT_MANAGER_H