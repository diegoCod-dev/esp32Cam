#include "wifi_server.h"
#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_http_server.h"
#include "sd.h" 

static const char *TAG = "wifi_server";

#define WIFI_AP_SSID "ESP32-CAM-SPIA"
#define WIFI_AP_PASS "12345678"

#define WIFI_STA_SSID "MOVISTAR WIFI6471"    // <-- nombre de tu router
#define WIFI_STA_PASS "976543210"    // <-- contraseña de tu router

// Estructura auxiliar para pasar el contexto de la petición HTTP al callback
typedef struct {
    httpd_req_t *req;
    bool sd_error;
} html_context_t;

// Función auxiliar que se ejecuta por cada archivo encontrado en la SD
static void add_file_to_html(const char *filename, void *arg) {
    html_context_t *ctx = (html_context_t *)arg;
    char html_item[256];
    snprintf(html_item, sizeof(html_item), 
             "<p><a href='/download?file=%s' target='_blank'>📥 Ver / Descargar %s</a></p>", 
             filename, filename);
    httpd_resp_send_chunk(ctx->req, html_item, HTTPD_RESP_USE_STRLEN);
}

// 1. Manejador para descargar una foto específica
static esp_err_t download_get_handler(httpd_req_t *req) {
    char filepath[64];
    char query[64];
    FILE *f = NULL;
    esp_err_t res = ESP_FAIL;

    // 1. Obtener los parámetros de la URL
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) != ESP_OK) {
        httpd_resp_send_404(req);
        goto cleanup;
    }

    char param[32];
    if (httpd_query_key_value(query, "file", param, sizeof(param)) != ESP_OK) {
        httpd_resp_send_404(req);
        goto cleanup;
    }

    snprintf(filepath, sizeof(filepath), "/sdcard/%s", param);

    // 2. Abrir el archivo desde la SD
    f = fopen(filepath, "r");
    if (!f) {
        httpd_resp_send_404(req);
        res = ESP_OK; // No es un fallo del servidor, el archivo no existe
        goto cleanup;
    }

    httpd_resp_set_type(req, "image/jpeg");
    char chunk[1024];
    size_t chunksize;
    
    // 3. Bucle de envío por bloques (chunks)
    while (true) {
        chunksize = fread(chunk, 1, sizeof(chunk), f);
        if (chunksize == 0) {
            break; // Fin del archivo alcanzado con éxito
        }

        // Si el cliente se desconecta, httpd_resp_send_chunk fallará
        if (httpd_resp_send_chunk(req, chunk, chunksize) != ESP_OK) {
            ESP_LOGW(TAG, "Conexión interrumpida por el cliente durante la descarga.");
            goto cleanup; // Salimos inmediatamente a limpiar
        }
    }

    // Indicar el fin de la respuesta chunked con un chunk vacío de tamaño 0
    httpd_resp_send_chunk(req, NULL, 0);
    res = ESP_OK;

cleanup:
    // 4. Zona de limpieza garantizada (se ejecuta siempre)
    if (f != NULL) {
        fclose(f);
        ESP_LOGI(TAG, "Archivo cerrado correctamente.");
    }
    return res;
}


// 2. Manejador principal que delega la lectura al componente SD
static esp_err_t index_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    
    httpd_resp_send_chunk(req, "<html><head><meta charset='utf-8'><title>ESP32-CAM Espia</title></head>", HTTPD_RESP_USE_STRLEN);
    httpd_resp_send_chunk(req, "<body style='font-family: Arial; text-align: center; margin-top: 20px;'>", HTTPD_RESP_USE_STRLEN);
    httpd_resp_send_chunk(req, "<h2>Galería de Fotos - ESP32-CAM</h2>", HTTPD_RESP_USE_STRLEN);
    httpd_resp_send_chunk(req, "<hr style='width: 50%;'>", HTTPD_RESP_USE_STRLEN);

    html_context_t ctx = { .req = req, .sd_error = false };
    
    // Llamamos a la función modular del componente SD
    sd_list_images(add_file_to_html, &ctx);

    httpd_resp_send_chunk(req, "</body></html>", HTTPD_RESP_USE_STRLEN);
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}


// 3. Manejador de prueba (Dummy) para verificar si el servidor responde
static esp_err_t test_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/plain");
    httpd_resp_send(req, "¡Hola desde la ruta dummy del ESP32-CAM!", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static const httpd_uri_t test_uri = {
    .uri       = "/test",
    .method    = HTTP_GET,
    .handler   = test_handler,
    .user_ctx  = NULL
};

static const httpd_uri_t root = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = index_handler,
    .user_ctx  = NULL
};

static const httpd_uri_t download = {
    .uri       = "/download",
    .method    = HTTP_GET,
    .handler   = download_get_handler,
    .user_ctx  = NULL
};

void start_webserver(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &root);
        httpd_register_uri_handler(server, &download);
        httpd_register_uri_handler(server, &test_uri); // <-- Registramos la ruta dummy
        ESP_LOGI(TAG, "Servidor web iniciado y rutas registradas.");
    } else {
        ESP_LOGE(TAG, "Error al iniciar el servidor web");
    }
}

void wifi_init_softap(void) {
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    // Crear las dos interfaces de red (AP y Estación)
    esp_netif_create_default_wifi_ap();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Configuración de tu red propia (AP)
    wifi_config_t wifi_ap_config = {
        .ap = {
            .ssid = WIFI_AP_SSID,
            .ssid_len = strlen(WIFI_AP_SSID),
            .channel = 1,
            .password = WIFI_AP_PASS,
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA2_PSK
        },
    };

    // Configuración para conectarse al router de casa (STA)
    wifi_config_t wifi_sta_config = {
        .sta = {
            .ssid = WIFI_STA_SSID,
            .password = WIFI_STA_PASS,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_ap_config));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_sta_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    // Forzar la conexión de la parte STA al router
    ESP_ERROR_CHECK(esp_wifi_connect());

    ESP_LOGI(TAG, "Wi-Fi APSTA iniciado. AP: %s | Conectando a STA: %s", WIFI_AP_SSID, WIFI_STA_SSID);
}

/* void wifi_init_softap(void) {
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = WIFI_SSID,
            .ssid_len = strlen(WIFI_SSID),
            .channel = 1,
            .password = WIFI_PASS,
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA2_PSK
        },
    };
    if (strlen(WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Wi-Fi AP iniciado. SSID: %s", WIFI_SSID);
}*/


