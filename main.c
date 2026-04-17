#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gptimer.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include <stdbool.h>

#define LED 18

static const char *TAG = "GPTIMER_ISR";

bool led_state = 1;

// Simple flag updated in ISR
volatile bool timer_flag = false;

// ISR callback
static bool IRAM_ATTR timer_on_alarm_cb(gptimer_handle_t timer,
                                        const gptimer_alarm_event_data_t *edata,
                                        void *user_ctx)
{
    // Minimal ISR work
    timer_flag = true;
    gpio_set_level(LED, led_state);
    led_state = !led_state;
    
    // Return whether a high-priority task should be woken (false = no)
    return false;
}

void app_main(void)
{   
    gpio_config_t bertie =
    {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL<<LED
    };
    gpio_config(&bertie);


    gptimer_handle_t timer = NULL;

    // 1. Configure timer
    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1000000, // 1 MHz = 1 tick = 1 µs
    };

    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &timer));

    // 2. Register ISR callback
    gptimer_event_callbacks_t cbs = {
        .on_alarm = timer_on_alarm_cb,
    };

    ESP_ERROR_CHECK(gptimer_register_event_callbacks(timer, &cbs, NULL));

    // 3. Configure alarm
    gptimer_alarm_config_t alarm_config = {
        .alarm_count = 1000000, // 1 second
        .reload_count = 0,
        .flags.auto_reload_on_alarm = true,
    };

    ESP_ERROR_CHECK(gptimer_set_alarm_action(timer, &alarm_config));

    // 4. Enable + start timer
    ESP_ERROR_CHECK(gptimer_enable(timer));
    ESP_ERROR_CHECK(gptimer_start(timer));

    // 5. Main loop
    while (1) {
        if (timer_flag) {
            timer_flag = false;
            ESP_LOGI(TAG, "Timer ISR fired");
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}