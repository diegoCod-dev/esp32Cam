/**
 * @file sd.h
 * @brief Componente reutilizable para la gestión y montaje de tarjetas SD en ESP-IDF.
 * 
 * Este módulo permite inicializar el sistema de archivos FAT sobre una tarjeta SD,
 * guardar archivos de imagen de forma segura mediante exclusión mutua (mutex),
 * y listar los archivos almacenados utilizando funciones de retrollamada (callback).
 */

#ifndef SD_H
#define SD_H

#include <stddef.h>
#include "esp_err.h"

/**
 * @brief Estructura de configuración para la inicialización de la tarjeta SD.
 */
typedef struct {
    const char *mount_point;      ///< Ruta del punto de montaje en el VFS (ej. "/sdcard")
    int max_files;                ///< Número máximo de archivos abiertos simultáneamente
    size_t allocation_unit_size;  ///< Tamaño de la unidad de asignación para FAT
} sd_config_t;

/**
 * @brief Tipo de función de retrollamada (callback) para procesar los archivos listados.
 * 
 * @param filename Nombre del archivo encontrado en el directorio.
 * @param arg Puntero a datos adicionales opcionales pasados por el usuario.
 */
typedef void (*sd_file_callback_t)(const char *filename, void *arg);

/**
 * @brief Inicializa el sistema de archivos y monta la tarjeta SD.
 * 
 * @param config Puntero a la estructura de configuración. Si se pasa NULL, 
 *               se utilizarán los valores por defecto (`/sdcard`, 5 archivos máximos).
 * @return esp_err_t 
 *         - ESP_OK si se montó con éxito.
 *         - Código de error de ESP-IDF en caso contrario.
 */
esp_err_t sd_logger_init(const sd_config_t *config);

/**
 * @brief Guarda un búfer de datos (como un fotograma de cámara) en la tarjeta SD.
 * 
 * Utiliza un mecanismo de exclusión mutua (mutex) para garantizar seguridad en entornos multihilo.
 * 
 * @param data Puntero al búfer de datos que se desea escribir.
 * @param len Longitud de los datos en bytes.
 * @param picture_number Número identificador de la foto para construir el nombre del archivo.
 * @return esp_err_t 
 *         - ESP_OK si el archivo se guardó correctamente.
 *         - ESP_FAIL si el recurso estaba ocupado o hubo un error de escritura.
 */
esp_err_t sd_logger_save_image(const uint8_t *data, size_t len, int picture_number);

/**
 * @brief Recorre el directorio de la tarjeta SD y ejecuta un callback por cada archivo compatible.
 * 
 * Filtra y busca archivos con extensión `.jpg`.
 * 
 * @param callback Función de retrollamada a ejecutar por cada archivo encontrado.
 * @param arg Puntero opcional de contexto para pasar al callback.
 */
void sd_list_images(sd_file_callback_t callback, void *arg);

#endif // SD_H