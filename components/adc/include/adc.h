#ifndef ADC_H
#define ADC_H

// =============================
// Header Files (Your Toolbox)
// =============================

    /* --- General --- */
    // #include "freertos/FreeRTOS.h"
    // #include "freertos/task.h"
    // #include "esp_log.h"
    // #include "esp_err.h"

    /* --- ADC --- */
    #include "esp_adc/adc_oneshot.h"    // For ADC HW interation
    #include "esp_adc/adc_cali.h"       // For voltage calibration

// =============================
// Application Log Tag
// =============================
   
    #define ADC_TAG "ADC"

// =============================
// ADC Configuration (Exposed for ADC.c)
// =============================
#define ADC_UNIT       ADC_UNIT_1
#define ADC_CHANNEL    ADC_CHANNEL_6   // GPIO34
#define BUFFER_SIZE    256             // Circular buffer length
#define ADC_SAMPLE_PERIOD_MS 10       // Sampling period (ms)
#define SAMPLE_RATE_HZ (1000 / ADC_SAMPLE_PERIOD_MS)  // Derived rate


// =============================
// Global Declarations (For cross-file access)
// =============================
extern adc_oneshot_unit_handle_t adc_handle;  // ADC driver handle
extern adc_cali_handle_t adc_cali_handle;     // ADC Calibration handle


// =============================
// Main Functions:
// =============================

    // =============================
    /* ADC Unit Initialization + Channel Configuration + Calibration */
    // ============================= 
    adc_oneshot_unit_handle_t init_adc(void);
    // Returns a handle to the initialized ADC unit.

    // =============================
    // FreeRTOS Task: ADC Sampling
    // =============================
    void adc_sampling(void *arg);



#endif // ADC_H