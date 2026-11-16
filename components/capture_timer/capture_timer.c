/**
 * @file capture_timer.c
 * @brief Implementación de las funciones del temporizador de captura.
 */

#include "capture_timer.h"
#include "esp_timer.h"

/** @brief Marca de tiempo en milisegundos del último reinicio del temporizador. */
static int64_t last_reset_ms = 0;

/** @brief Intervalo de tiempo configurado en milisegundos. */
static uint32_t configured_timeout_ms = 0;

/**
 * @brief Obtiene el tiempo actual del sistema en milisegundos.
 * 
 * Convierte el tiempo obtenido por esp_timer_get_time() (en microsegundos) a milisegundos.
 * 
 * @return int64_t Tiempo actual en milisegundos.
 */
static int64_t get_time_ms(void)
{
    return esp_timer_get_time() / 1000;
}

/**
 * @brief Inicializa el temporizador de captura con un intervalo de tiempo.
 */
void capture_timer_init(uint32_t timeout_ms)
{
    configured_timeout_ms = timeout_ms;
    last_reset_ms = get_time_ms();
}

/**
 * @brief Reinicia el contador del temporizador.
 */
void capture_timer_reset(void)
{
    last_reset_ms = get_time_ms();
}

/**
 * @brief Verifica si el tiempo configurado ha expirado.
 */
bool capture_timer_expired(void)
{
    int64_t elapsed_ms;

    elapsed_ms = get_time_ms() - last_reset_ms;

    if (elapsed_ms >= configured_timeout_ms)
    {
        return true;
    }

    return false;
}