#ifndef CAPTURE_TIMER_H
#define CAPTURE_TIMER_H

#include <stdbool.h>
#include <stdint.h>

// Inicializa el temporizador
void capture_timer_init(uint32_t timeout_ms);

// Reinicia el contador
void capture_timer_reset(void);

// Devuelve true si se cumplio el tiempo configurado
bool capture_timer_expired(void);

#endif