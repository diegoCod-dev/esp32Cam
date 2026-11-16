/**
 * @file sd.c
 * @brief Implementación del componente reutilizable de tarjeta SD con VFS FAT.
 */

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

/** @brief Mutex global para la sincronización y protección de acceso concurrente a la SD. */
static SemaphoreHandle_t sd_mutex = NULL;

/** @brief Almacena dinámicamente el punto de montaje activo. */
static char current_mount_point[32] = "/sdcard";

/**
 * @brief Inicializa el sistema de archivos y monta la tarjeta SD con configuración personalizada.
 */
esp_err_t sd_logger_init(const sd_config_t *config) {
    ESP_LOGI(TAG, "Inicializando tarjeta SD...");

    // Crear el mutex si no existe
    if (sd_mutex == NULL) {
        sd_mutex = xSemaphoreCreateMutex();
        if (sd_mutex == NULL) {
            ESP_LOGE(TAG, "No se pudo crear el mutex para la tarjeta SD");
            return ESP_ERR_NO_MEM;
        }
    }

    // Valores por defecto si la configuración es nula
    const char *mount_pt = "/sdcard";
    int max_files = 5;
    size_t alloc_unit = 16 * 1024;

    if (config != NULL) {
        if (config->mount_point != NULL) {
            mount_pt = config->mount_point;
        }
        if (config->max_files > 0) {
            max_files = config->max_files;
        }
        if (config->allocation_unit_size > 0) {
            alloc_unit = config->allocation_unit_size;
        }
    }

    // Guardar de forma segura el punto de montaje
    strncpy(current_mount_point, mount_pt, sizeof(current_mount_point) - 1);
    current_mount_point[sizeof(current_mount_point) - 1] = '\0';

    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = max_files,
        .allocation_unit_size = alloc_unit
    };

    sdmmc_card_t *card;
    esp_err_t ret = esp_vfs_fat_sdmmc_mount(current_mount_point, &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Fallo al montar la tarjeta SD en %s (%s).", current_mount_point, esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "¡Tarjeta SD montada con éxito en %s!", current_mount_point);
    sdmmc_card_print_info(stdout, card);
    return ESP_OK;
}

/**
 * @brief Guarda un búfer de datos en la tarjeta SD de forma segura mediante mutex.
 */
esp_err_t sd_logger_save_image(const uint8_t *data, size_t len, int picture_number) {
    FILE *file = NULL;
    esp_err_t ret = ESP_FAIL;

    if (xSemaphoreTake(sd_mutex, pdMS_TO_TICKS(2000)) != pdTRUE) {
        ESP_LOGE(TAG, "SD ocupada, no se pudo guardar la foto");
        return ESP_FAIL;
    }

    char path[64];
    snprintf(path, sizeof(path), "%s/foto_%d.jpg", current_mount_point, picture_number);

    file = fopen(path, "w");
    if (file == NULL) {
        ESP_LOGE(TAG, "Error al abrir el archivo %s en la SD para escritura", path);
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

/**
 * @brief Recorre el directorio de la SD buscando archivos .jpg.
 */
void sd_list_images(sd_file_callback_t callback, void *arg) {
    if (callback == NULL) {
        return;
    }

    if (xSemaphoreTake(sd_mutex, pdMS_TO_TICKS(2000)) != pdTRUE) {
        ESP_LOGE(TAG, "SD ocupada, no se pudo listar");
        return;
    }
    
    DIR *dir = opendir(current_mount_point);
    if (dir == NULL) {
        ESP_LOGE(TAG, "No se pudo abrir el directorio %s de la tarjeta SD", current_mount_point);
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