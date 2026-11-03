#include "sd.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"

static const char *TAG = "sd_logger";

esp_err_t sd_logger_init(void) {
    ESP_LOGI(TAG, "Inicializando tarjeta SD...");
    
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
    char path[64];
    snprintf(path, sizeof(path), "/sdcard/foto_%d.jpg", picture_number);

    FILE *file = fopen(path, "w");
    if (file == NULL) {
        ESP_LOGE(TAG, "Error al abrir el archivo en la SD para escritura");
        return ESP_FAIL;
    }

    fwrite(data, 1, len, file);
    fclose(file);
    
    ESP_LOGI(TAG, "¡Foto guardada en SD: %s (%u bytes)!", path, len);
    return ESP_OK;
}