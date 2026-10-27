#ifndef ESP_CAM_H
#define ESP_CAM_H

#include "esp_err.h"
#include "esp_camera.h"

// Prototipos de funciones públicas del componente
esp_err_t esp_cam_init(void);
camera_fb_t* esp_cam_capture(void);
void esp_cam_return(camera_fb_t *fb);

#endif