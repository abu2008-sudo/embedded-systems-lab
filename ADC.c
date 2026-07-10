#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "S01_ADC";

#define MAX_HISTORY 32

typedef struct
{
    adc_channel_t channel;
    adc_unit_t unit;
    char label[16];
    int reading;

    int global_min;
    int global_max;

    int range_min;
    int range_max;
    int avg;

    int history[MAX_HISTORY];   // always reserved at max size — memory-safe, no heap
    int buffer_size;            // active window size for this channel (<= MAX_HISTORY)
    int histry_indx;
    int history_counts;
} ADC_Channel_t;

void ADC_Channel_init(ADC_Channel_t *ch, adc_oneshot_unit_handle_t handle)
{
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(handle, ch->channel, &config));
}

// buffer_size is the user-requested history window size. If it exceeds
// MAX_HISTORY, we log the mistake loudly and cap it to MAX_HISTORY — the
// closest safe value to what was actually requested, rather than silently
// ignoring intent or crashing.
ADC_Channel_t ADC_Channel_create(adc_channel_t channel, adc_unit_t unit, const char *label, int buffer_size)
{
    ADC_Channel_t ch;
    ch.channel = channel;
    ch.unit = unit;
    strcpy(ch.label, label);
    ch.reading = 0;

    if (buffer_size > MAX_HISTORY) {
        ESP_LOGE(TAG, "[%s] Requested buffer_size %d exceeds MAX_HISTORY %d — capping to %d",
                 label, buffer_size, MAX_HISTORY, MAX_HISTORY);
        buffer_size = MAX_HISTORY;
    }
    if (buffer_size <= 0) {
        ESP_LOGE(TAG, "[%s] Invalid buffer_size %d — defaulting to 8", label, buffer_size);
        buffer_size = 8;
    }
    ch.buffer_size = buffer_size;

    // Global (all-time) stats — must start at true extremes since they're
    // never reset, only ever compared and conditionally updated.
    ch.global_min = 4095;
    ch.global_max = 0;

    // Range-based stats — reset fresh every call inside ADC_update_range_stats(),
    // so their starting value here doesn't actually matter, but zeroed for cleanliness.
    ch.range_min = 4095;
    ch.range_max = 0;
    ch.avg = 0;

    ch.history_counts = 0;
    ch.histry_indx = 0;

    return ch;
}

void ADC_read(adc_oneshot_unit_handle_t handle, ADC_Channel_t *ch)
{
    ESP_ERROR_CHECK(adc_oneshot_read(handle, ch->channel, &ch->reading));
}

// Pushes the latest reading into the circular buffer (write pointer + valid count only).
// Bounds are now driven by ch->buffer_size instead of a hardcoded 8.
void ADC_update_stats(ADC_Channel_t *ch)
{
    ch->history[ch->histry_indx] = ch->reading;
    ch->histry_indx++;
    if (ch->histry_indx == ch->buffer_size) { ch->histry_indx = 0; }
    if (ch->history_counts < ch->buffer_size) { ch->history_counts++; }
}

// Recomputes range_min, range_max, avg from only the valid entries in the
// circular buffer (up to ch->buffer_size), then updates global_min/global_max
// if this window set a new all-time record.
void ADC_update_range_stats(ADC_Channel_t *ch)
{
    int range_min = 4095;
    int range_max = 0;
    int sum = 0;

    if (ch->history_counts == 0) return;

    for (int i = 0; i < ch->history_counts; i++) {
        if (ch->history[i] < range_min) { range_min = ch->history[i]; }
        if (ch->history[i] > range_max) { range_max = ch->history[i]; }
        sum += ch->history[i];
    }

    ch->range_min = range_min;
    ch->range_max = range_max;
    ch->avg = sum / ch->history_counts;

    if (range_min < ch->global_min) { ch->global_min = range_min; }
    if (range_max > ch->global_max) { ch->global_max = range_max; }
}

int ADC_get_stat(ADC_Channel_t *ch, const char *stat)
{
    if (strcmp(stat, "range_min") == 0) { return ch->range_min; }
    if (strcmp(stat, "range_max") == 0) { return ch->range_max; }
    if (strcmp(stat, "global_min") == 0) { return ch->global_min; }
    if (strcmp(stat, "global_max") == 0) { return ch->global_max; }
    if (strcmp(stat, "avg") == 0) { return ch->avg; }
    return -1;
}

void ADC_log(ADC_Channel_t *ch)
{
    ESP_LOGI(TAG, "[%s] Raw: %d", ch->label, ch->reading);
    ESP_LOGI(TAG, "[%s] Range(N=%d) Min: %d  Max: %d  Avg: %d",
             ch->label, ch->buffer_size,
             ADC_get_stat(ch, "range_min"),
             ADC_get_stat(ch, "range_max"),
             ADC_get_stat(ch, "avg"));
    ESP_LOGI(TAG, "[%s] Global Min: %d  Global Max: %d",
             ch->label,
             ADC_get_stat(ch, "global_min"),
             ADC_get_stat(ch, "global_max"));
}

void app_main(void)
{
    printf("app_main started\n");
    adc_oneshot_unit_handle_t adc1_handle;

    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    ADC_Channel_t ch[3];
    // Each channel can now choose its own history window size, up to MAX_HISTORY (32).
    ch[0] = ADC_Channel_create(ADC_CHANNEL_6, ADC_UNIT_1, "Pot1", 8);
    ch[1] = ADC_Channel_create(ADC_CHANNEL_7, ADC_UNIT_1, "Pot2", 16);
    ch[2] = ADC_Channel_create(ADC_CHANNEL_4, ADC_UNIT_1, "Pot3", 50); // intentionally too large -> capped + logged

    for (int i = 0; i < 3; i++) {
        ADC_Channel_init(&ch[i], adc1_handle);
    }

    while (1) {
        for (int i = 0; i < 3; i++) {
            ADC_read(adc1_handle, &ch[i]);
            ADC_update_stats(&ch[i]);
            ADC_update_range_stats(&ch[i]);
            ADC_log(&ch[i]);
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
