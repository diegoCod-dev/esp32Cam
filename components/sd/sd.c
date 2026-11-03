#include "sd.h"
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static const char *TAG = "sd_module";

#define MOUNT_POINT "/sdcard"

static SemaphoreHandle_t sd_mutex = NULL;

esp_err_t sd_logger_init(void) {
    ESP_LOGI(TAG, "Inicializando tarjeta SD...");

    // Crear el mutex si no existe
    if (sd_mutex == NULL) {
        sd_mutex = xSemaphoreCreateMutex();
    }

    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

    sdmmc_card_t *card;
    esp_err_t ret = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Fallo al montar la tarjeta SD (%s).", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "¡Tarjeta SD montada con éxito!");
    sdmmc_card_print_info(stdout, card);
    return ESP_OK;
}

esp_err_t sd_logger_save_image(const uint8_t *data, size_t len, int picture_number) {
    FILE *file = NULL;
    esp_err_t ret = ESP_FAIL;

    if (xSemaphoreTake(sd_mutex, pdMS_TO_TICKS(2000)) != pdTRUE) {
        ESP_LOGE(TAG, "SD ocupada, no se pudo guardar la foto");
        return ESP_FAIL;
    }

    char path[64];
    snprintf(path, sizeof(path), "/sdcard/foto_%d.jpg", picture_number);

    file = fopen(path, "w");
    if (file == NULL) {
        ESP_LOGE(TAG, "Error al abrir el archivo en la SD para escritura");
        goto cleanup;
    }

    if (fwrite(data, 1, len, file) != len) {
        ESP_LOGE(TAG, "Error al escribir los datos en la SD");
        goto cleanup;
    }

    ESP_LOGI(TAG, "¡Foto guardada en SD: %s (%u bytes)!", path, len);
    ret = ESP_OK;

cleanup:
    if (file != NULL) {
        fclose(file);
    }
    xSemaphoreGive(sd_mutex);
    return ret;
}

// Función para recorrer el directorio de la SD
void sd_list_images(sd_file_callback_t callback, void *arg) {
    if (xSemaphoreTake(sd_mutex, pdMS_TO_TICKS(2000)) != pdTRUE) {
        ESP_LOGE(TAG, "SD ocupada, no se pudo listar");
        return;
    }
    
    DIR *dir = opendir(MOUNT_POINT);
    if (dir == NULL) {
        ESP_LOGE(TAG, "No se pudo abrir el directorio de la tarjeta SD");
        xSemaphoreGive(sd_mutex);
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // Filtrar únicamente los archivos que contengan la extensión .jpg
        if (strstr(entry->d_name, ".jpg") != NULL) {
            callback(entry->d_name, arg);
        }
    }
    closedir(dir);
    xSemaphoreGive(sd_mutex);
}