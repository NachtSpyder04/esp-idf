/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/rmt_tx.h"
#include "dshot_esc_encoder.h"

#if CONFIG_IDF_TARGET_ESP32H2
#define DSHOT_ESC_RESOLUTION_HZ 32000000 // 32MHz resolution, DSHot protocol needs a relatively high resolution
#else
#define DSHOT_ESC_RESOLUTION_HZ 40000000 // 40MHz resolution, DSHot protocol needs a relatively high resolution
#endif

// pin defs for each ESC
#define DSHOT_ESC_GPIO_NUM1  12
#define DSHOT_ESC_GPIO_NUM2  13
#define DSHOT_ESC_GPIO_NUM3  14
#define DSHOT_ESC_GPIO_NUM4  15

static const char *TAG = "example";

void app_main(void)
{
    ESP_LOGI(TAG, "Create RMT TX channels");

    rmt_channel_handle_t esc_chan[4]; // Array to store handles for all 4 ESC channels

    // Configure RMT channels for each ESC
    for (int i = 0; i < 4; i++) {
        rmt_tx_channel_config_t tx_chan_config = {
            .clk_src = RMT_CLK_SRC_DEFAULT, // select a clock that can provide the needed resolution
            .gpio_num = (i == 0) ? DSHOT_ESC_GPIO_NUM1 : (i == 1) ? DSHOT_ESC_GPIO_NUM2 : (i == 2) ? DSHOT_ESC_GPIO_NUM3 : DSHOT_ESC_GPIO_NUM4,
            .mem_block_symbols = 64,
            .resolution_hz = DSHOT_ESC_RESOLUTION_HZ,
            .trans_queue_depth = 10, // set the number of transactions that can be pending in the background
        };
        ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_chan_config, &esc_chan[i]));
    }

    ESP_LOGI(TAG, "Install Dshot ESC encoders");

    rmt_encoder_handle_t dshot_encoder[4]; // Array to store handles for all 4 DSHOT encoders

    // Configure DSHOT encoders for each ESC
    for (int i = 0; i < 4; i++) {
        dshot_esc_encoder_config_t encoder_config = {
            .resolution = DSHOT_ESC_RESOLUTION_HZ,
            .baud_rate = 600000, // DSHOT600 protocol
            .post_delay_us = 50, // extra delay between each frame
        };
        ESP_ERROR_CHECK(rmt_new_dshot_esc_encoder(&encoder_config, &dshot_encoder[i]));
    }

    ESP_LOGI(TAG, "Enable RMT TX channels");

    // Enable RMT channels for all ESCs
    for (int i = 0; i < 4; i++) {
        ESP_ERROR_CHECK(rmt_enable(esc_chan[i]));
    }

    rmt_transmit_config_t tx_config = {
        .loop_count = -1, // infinite loop
    };

    dshot_esc_throttle_t throttle[4]; // Array to store throttle values for all 4 ESCs

    ESP_LOGI(TAG, "Start ESCs by sending zero throttle for a while...");

    // Initialize throttle values for all ESCs to zero
    for (int i = 0; i < 4; i++) {
        throttle[i].throttle = 0;
        throttle[i].telemetry_req = false; // telemetry is not supported in this example
    }

    // Send zero throttle to all ESCs
    for (int i = 0; i < 4; i++) {
        ESP_ERROR_CHECK(rmt_transmit(esc_chan[i], dshot_encoder[i], &throttle[i], sizeof(throttle[i]), &tx_config));
    }
    vTaskDelay(pdMS_TO_TICKS(5000)); // Wait for 5 seconds

    ESP_LOGI(TAG, "Increase throttle, no telemetry");

    // Increase throttle for all ESCs
    for (uint16_t thro = 100; thro < 1000; thro += 10) {
        for (int i = 0; i < 4; i++) {
            throttle[i].throttle = thro;
            ESP_ERROR_CHECK(rmt_transmit(esc_chan[i], dshot_encoder[i], &throttle[i], sizeof(throttle[i]), &tx_config));
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
