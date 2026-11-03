#ifndef SD_LOGGER_H
#define SD_LOGGER_H

#include <stddef.h>
#include "esp_err.h"

// Inicializa el sistema de archivos y monta la tarjeta SD
esp_err_t sd_logger_init(void);

// Guarda un búfer de imagen (como el fotograma de la cámara) en la SD
esp_err_t sd_logger_save_image(const uint8_t *data, size_t len, int picture_number);

// función para listar fotos y pasarlas a una función de retorno (callback)
typedef void (*sd_file_callback_t)(const char *filename, void *arg);
void sd_list_images(sd_file_callback_t callback, void *arg);

#endif