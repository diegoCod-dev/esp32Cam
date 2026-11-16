/**
 * @file capture_timer.h
 * @brief Temporizador basado en esp_timer para control de intervalos de captura.
 * 
 * Este módulo proporciona funciones para inicializar, reiniciar y verificar
 * el cumplimiento de un intervalo de tiempo especificado en milisegundos,
 * ideal para tareas periódicas en ESP-IDF sin bloquear el sistema.
 */

#ifndef CAPTURE_TIMER_H
#define CAPTURE_TIMER_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Inicializa el temporizador de captura con un intervalo de tiempo.
 * 
 * Configura el tiempo de espera deseado y toma la marca de tiempo actual 
 * como punto de partida (referencia inicial).
 * 
 * @param timeout_ms Intervalo de tiempo en milisegundos para la expiración.
 */
void capture_timer_init(uint32_t timeout_ms);

/**
 * @brief Reinicia el contador del temporizador.
 * 
 * Actualiza la última marca de tiempo registrada al momento actual, 
 * reiniciando la cuenta para el próximo intervalo.
 */
void capture_timer_reset(void);

/**
 * @brief Verifica si el tiempo configurado ha expirado.
 * 
 * Calcula el tiempo transcurrido desde la última inicialización o reseteo 
 * comparándolo con el intervalo configurado.
 * 
 * @return true Si el tiempo transcurrido es mayor o igual al configurado.
 * @return false Si aún no se ha cumplido el tiempo.
 */
bool capture_timer_expired(void);

#endif // CAPTURE_TIMER_H