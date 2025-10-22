// =============================
// Header Files (Your Toolbox)
// =============================

    /* --- General --- */
    #include "esp_log.h"                    // Serial console output
    #include "esp_err.h"                    // ESP Error codes  
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"

    /* --- ADC --- */
    #include "adc.h"




// =============================
// Main Application Entry Point
// =============================
void app_main(void)
{   

    // --- Start logging ---
    // ESP-IDF functon to print to serial console
    ESP_LOGI(ADC_TAG, "Starting ADC Initialization and Calibration...");

    // --- Initialize ADC ---
    // ESP-IDF function to Configures ADC Unit 1, Channel 6 (GPIO34) and sets up calibration
    // Returns a handle that uniquely identifies this ADC configuration.
    adc_handle = init_adc();
    if (!adc_handle) {
        ESP_LOGE(ADC_TAG, "ADC initialization failed. Exiting.");
        return;
    }

    BaseType_t task_status;

    // --- Task for ADC Sampling ---
    task_status = xTaskCreate(adc_sampling, "ADC Sampling", 2048, NULL, 5, NULL);
    if (task_status != pdPASS)
        ESP_LOGE(ADC_TAG, "Failed to create ADC sampling task!");

}







