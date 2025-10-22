// =============================
// Header Files (Your Toolbox)
// =============================

    /* --- General --- */
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
    #include "esp_log.h"
    #include "esp_err.h"

    /* --- ADC --- */
    // #include "esp_adc/adc_oneshot.h"    // For ADC HW interation
    // #include "esp_adc/adc_cali.h"       // For voltage calibration
    #include "adc.h"
    // #include <math.h>  // For Goertzel (sin/cos)


// =============================
// ADC Globals Definition (Here, for Module Ownership)
// =============================
adc_oneshot_unit_handle_t adc_handle = NULL;  // ADC driver handle
adc_cali_handle_t adc_cali_handle = NULL;     // ADC Calibration handle
int16_t adc_buffer[BUFFER_SIZE];
volatile size_t buffer_index = 0;
// volatile uint32_t blink_count = 0;
// volatile uint8_t attention_level = 0;

// IIR Bandpass Globals (2nd-order Butterworth, 0.5-30Hz @100Hz sample)
// static float bp_a[3] = {1.0f, -1.927f, 0.930f};  // Denom coeffs
// static float bp_b[3] = {0.004f, 0.0f, -0.004f};  // Num coeffs (approx for fs=100Hz)
// float bp_x[2] = {0};  // Input history
// float bp_y[2] = {0};  // Output history

// =============================
// ADC Unit Initialization + Channel Configuration + Calibration
// =============================
    // Function to initialize the ADC unit, configure the channel, and set up calibration.
    // Returns a handle to the initialized ADC unit.
    adc_oneshot_unit_handle_t init_adc(void)
    {

        esp_err_t ret;

        // ==============================
        // 1. ADC Unit Configuration
        // ==============================

        // STEP 1A : Create a Handle (Done - Alrady defined globally)
        // A handle is like a "pointer" or reference to a software object that represents
        // the ADC hardware inside ESP-IDF. This handle will be used for all future ADC calls.
        // (At this point, adc_handle is just a NULL pointer.)
        // static adc_oneshot_unit_handle_t adc_handle;    // Reference to ADC driver

        // STEP 1B : Define the ADC Unit configuration structure
        // This structure describes global settings for the ADC peripheral.
        // - unit_id: Which ADC hardware block (ADC1 or ADC2)
        adc_oneshot_unit_init_cfg_t init_config = {
            .unit_id = ADC_UNIT,   // Use ADC1 block
        };
        // Nothing is initialized yet. This is just the **desired configuration**.

        // STEP 1C : Initialize ADC Unit using the ESP-IDF API
        // Function: adc_oneshot_new_unit(init_config, &adc_handle)
        // What it does:
        // 1. Allocates memory for the ADC driver object
        // 2. Programs ADC hardware registers according to init_config
        // 3. Updates adc_handle to point to this driver object
        ret = adc_oneshot_new_unit(&init_config, &adc_handle);
        if (ret == ESP_OK) {
            ESP_LOGI(ADC_TAG, "ADC Unit initialized successfully!");
        } else {
            ESP_LOGE(ADC_TAG, "Failed to initialize ADC unit! Error code: %d", ret);
            return NULL; // Stop if initialization failed
        }
        // Now adc_handle points to a fully initialized ADC driver object
        // but the ADC channel/pin and input scaling are not set yet.

        // ==============================
        // 2. ADC Channel Configuration
        // ==============================

        // STEP 2B : Define the Channel Configuration structure
        // This is a separate configuration structure that describes how to read from a specific ADC channel (pin).
        // - bitwidth: Resolution of conversion (default 12-bit)
        // - attenuation: How much input voltage the ADC can measure (~3.3V for DB_11)
        adc_oneshot_chan_cfg_t chan_config = {
            .bitwidth = ADC_BITWIDTH_DEFAULT,  // Default 12-bit resolution
            .atten = ADC_ATTEN_DB_12           // ~3.3V full-scale voltage range
        };

        // STEP 2C: [ BLANK]
        ret = adc_oneshot_config_channel(adc_handle, ADC_CHANNEL, &chan_config);
        if (ret == ESP_OK) {
            ESP_LOGI(ADC_TAG, "ADC channel configured successfully!");
        } else {
            ESP_LOGE(ADC_TAG, "Failed to configure ADC channel! Error code: %d", ret);
            return NULL;
        }

        // ==============================
        // 3. ADC Calibration Initialization
        // ==============================

        // Calibration is optional but recommended for accurate voltage readings.
        // On ESP32, raw ADC values may vary due to temperature, voltage supply, and manufacturing.
        // The calibration API converts raw readings to mV.

        // Step 3A: Define the Calibration configuration structure
        // This structure describes how the calibration should be performed.
        // - unit_id: Which ADC unit (must match the one used before)
        // - atten: Must match the attenuation used in channel config
        // - bitwidth: Must match the bitwidth used in channel config
        adc_cali_line_fitting_config_t cali_cfg = {
            .unit_id = ADC_UNIT,               // Same ADC unit as before
            .atten = ADC_ATTEN_DB_12,          // Same attenuation as channel config
            .bitwidth = ADC_BITWIDTH_DEFAULT   // Same bitwidth as channel config
        };

        // Step 3B: Initialize Calibration using ESP-IDF API
        // Function: adc_cali_create_scheme_curve_fitting(&cali_cfg, &handle)
        // What it does:
        // - Allocates memory for calibration object
        // - Prepares math to convert raw ADC → voltage (mV)
        if (adc_cali_create_scheme_line_fitting(&cali_cfg, &adc_cali_handle) == ESP_OK) {
            ESP_LOGI(ADC_TAG, "ADC calibration ready.");
        } else {
            ESP_LOGW(ADC_TAG, "ADC calibration not available. Using raw ADC values.");
            adc_cali_handle = NULL;          // Use raw values if calibration fails
        }

        // --- End of setup ---
        ESP_LOGI(ADC_TAG, "ADC is now initialized and ready for sampling.");

        // Return the ADC driver handle in case the caller wants it
        return adc_handle;

    }


// =============================
// FreeRTOS Task: ADC Sampling
// =============================
    void adc_sampling(void *arg)
    {

        ESP_LOGI(ADC_TAG, "ADC sampling task started!");

        while (1) {

            int raw = 0;
            int voltage = 0; // Calibrated voltage in mV

            // --- 1. Read raw ADC value ---
            adc_oneshot_read(adc_handle, ADC_CHANNEL, &raw); // ESP-IDF API

            // --- 2. Convert raw to calibrated voltage (mV) ---
            if (adc_cali_handle) {
                adc_cali_raw_to_voltage(adc_cali_handle, raw, &voltage); // ESP-IDF API
            } else {
                // Fallback if calibration unavailable
                voltage = raw;
            }

            // --- 3. Store calibrated voltage in circular buffer ---
            // Note: 1 unit = 0.1 mV scaling for EEG µV interpretation (e.g., 200 threshold = 20µV actual)
            adc_buffer[buffer_index] = (int16_t)(voltage * 10);
            buffer_index = (buffer_index + 1) % BUFFER_SIZE; // Wrap around

            // --- 4. Optional: Print to serial ---
            ESP_LOGI(ADC_TAG, "Raw ADC: %d mV -> Buffer[%zu]=%d", voltage, buffer_index, adc_buffer[buffer_index-1]);

            // --- 5. Delay for next sample (100 ms) ---
            vTaskDelay(pdMS_TO_TICKS(ADC_SAMPLE_PERIOD_MS));

        }
    }

