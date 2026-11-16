/**
 * @file wifi_server.h
 * @brief Servidor web local y gestión de red Wi-Fi en modo híbrido APSTA.
 * 
 * Este módulo inicializa simultáneamente un punto de acceso (Access Point) propio 
 * para visualizar la galería local de fotos y una conexión Station hacia el router 
 * del hogar, además de levantar un servidor HTTP con rutas de visualización y descarga.
 */

#ifndef WIFI_SERVER_H
#define WIFI_SERVER_H

/**
 * @brief Inicializa el subsistema Wi-Fi en modo híbrido APSTA (Access Point + Station).
 * 
 * Configura los parámetros de red y credenciales definidos en Kconfig, levantando 
 * la red propia del ESP32-CAM y conectándose al router doméstico para alcanzar el broker MQTT.
 */
void wifi_init_softap(void);

/**
 * @brief Inicia el servidor web HTTP y registra las rutas del sistema.
 * 
 * Configura el servidor HTTP por defecto de ESP-IDF y registra los manejadores 
 * para la galería principal (`/`), la descarga de imágenes (`/download`) y una ruta de prueba (`/test`).
 */
void start_webserver(void);

#endif // WIFI_SERVER_H