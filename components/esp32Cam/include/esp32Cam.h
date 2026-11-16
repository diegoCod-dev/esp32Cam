/**
 * @file esp32Cam.h
 * @brief Interfaz pública del componente de la cámara OV2640 para ESP32-CAM.
 * 
 * Define las funciones para inicializar el hardware de la cámara, capturar fotogramas 
 * (frame buffers) haciendo uso obligatorio de la PSRAM, y liberar los recursos asociados.
 */

#ifndef ESP_CAM_H
#define ESP_CAM_H

#include "esp_err.h"
#include "esp_camera.h"

/**
 * @brief Inicializa el hardware de la cámara OV2640 y asigna sus búferes en PSRAM.
 * 
 * Configura los pines GPIO específicos del modelo AI-Thinker, frecuencias de reloj XCLK,
 * formato de píxel (JPEG) y tamaños de búfer para garantizar estabilidad en las capturas.
 * 
 * @return esp_err_t 
 *         - ESP_OK si la cámara se inicializó correctamente.
 *         - Otro código de error de ESP-IDF en caso de fallo.
 */
esp_err_t esp_cam_init(void);

/**
 * @brief Captura un fotograma (frame buffer) utilizando la cámara.
 * 
 * @return camera_fb_t* Puntero a la estructura del búfer de la imagen capturada, 
 *                      o NULL si ocurrió un error en la captura.
 */
camera_fb_t* esp_cam_capture(void);

/**
 * @brief Libera el búfer de la cámara previamente capturado.
 * 
 * Es estrictamente obligatorio llamar a esta función después de procesar o guardar 
 * cada foto para devolver el búfer a la cola de la cámara y evitar fugas de memoria.
 * 
 * @param fb Puntero al frame buffer (`camera_fb_t`) que se desea liberar.
 */
void esp_cam_return(camera_fb_t *fb);

#endif // ESP_CAM_H