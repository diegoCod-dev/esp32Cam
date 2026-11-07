#include "capture_timer.h"

#include "esp_timer.h"

static int64_t last_reset_ms = 0;
static uint32_t configured_timeout_ms = 0;


static int64_t get_time_ms(void)
{
    return esp_timer_get_time() / 1000;
}


void capture_timer_init(uint32_t timeout_ms)
{
    configured_timeout_ms = timeout_ms;
    last_reset_ms = get_time_ms();
}


void capture_timer_reset(void)
{
    last_reset_ms = get_time_ms();
}


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