    #include "esp_adc/adc_oneshot.h"
    #include "esp_log.h"
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
    static const char *TAG = "S01_ADC";
    void app_main(void) {
        adc_oneshot_unit_handle_t adc1_handle;

        adc_oneshot_unit_init_cfg_t init_config1 = {
            .unit_id = ADC_UNIT_1,
            .ulp_mode = ADC_ULP_MODE_DISABLE,
        };
        ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

        adc_oneshot_chan_cfg_t config = {
            .bitwidth = ADC_BITWIDTH_DEFAULT,
            .atten = ADC_ATTEN_DB_12,
        };
        ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_6, &config));
        // Declare a single integer variable for the reading
        // Read and log GPIO 34 directly using its specific hardware channel (ADC1 Channel 6)
        while(1)
        {
            int adc_raw_single = 0;
            ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, ADC_CHANNEL_6, &adc_raw_single));
            ESP_LOGI(TAG, "ADC%d Channel[%d] Raw Data: %d", ADC_UNIT_1 + 1, ADC_CHANNEL_6, adc_raw_single);
            vTaskDelay(pdMS_TO_TICKS(500));
        };

    }