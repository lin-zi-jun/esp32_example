/* board.c - Board-specific hooks */

/*
 * Copyright (c) 2017 Intel Corporation
 * Additional Copyright (c) 2018 Espressif Systems (Shanghai) PTE LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

#include "driver/uart.h"
#include "esp_log.h"

#include "iot_button.h"
#include "board.h"
#include "esp_coexist.h"
#include "esp_ble_mesh_common_api.h"

#define TAG "BOARD"

#if defined(CONFIG_BLE_MESH_ESP_WROOM_32)
#define BUTTON_IO_NUM           0
#else
#define BUTTON_IO_NUM           2
#endif
#define BUTTON_ACTIVE_LEVEL     0

extern void example_ble_mesh_send_test_msg(bool resend);
extern void example_ble_mesh_test_init(void);
extern esp_err_t example_ble_mesh_start(void);
extern bool example_deinit_test;

struct _led_state led_state[3] = {
    { LED_OFF, LED_OFF, LED_R, "red"   },
    { LED_OFF, LED_OFF, LED_G, "green" },
    { LED_OFF, LED_OFF, LED_B, "blue"  },
};

void board_led_operation(uint8_t pin, uint8_t onoff)
{
    for (int i = 0; i < ARRAY_SIZE(led_state); i++) {
        if (led_state[i].pin != pin) {
            continue;
        }
        if (onoff == led_state[i].previous) {
            ESP_LOGW(TAG, "led %s is already %s",
                led_state[i].name, (onoff ? "on" : "off"));
            return;
        }
        gpio_set_level(pin, onoff);
        led_state[i].previous = onoff;
        return;
    }
    ESP_LOGE(TAG, "LED is not found!");
}

static void board_led_init(void)
{
    for (int i = 0; i < ARRAY_SIZE(led_state); i++) {
        gpio_pad_select_gpio(led_state[i].pin);
        gpio_set_direction(led_state[i].pin, GPIO_MODE_OUTPUT);
        gpio_set_level(led_state[i].pin, LED_OFF);
        led_state[i].previous = LED_OFF;
    }
}

static void button_tap_cb(void* arg)
{
    ESP_LOGI(TAG, "tap cb (%s)", (char *)arg);

    if (example_deinit_test == true) {
        static uint8_t count;
        if (count++ % 2 == 0) {
            esp_ble_mesh_deinit_param_t param = {
                .erase_flash = false,
            };
            if (esp_ble_mesh_deinit(&param) != ESP_OK) {
                ESP_LOGE(TAG, "%s, BLE Mesh deinit failed", __func__);
            } else {
                ESP_LOGW(TAG, "BLE Mesh deinit");
            }
        } else {
            if (example_ble_mesh_start() != ESP_OK) {
                ESP_LOGE(TAG, "%s, BLE Mesh start failed", __func__);
            }
        }
    } else {
        esp_coex_status_bit_clear(ESP_COEX_ST_TYPE_BLE, ESP_COEX_BLE_ST_MESH_CONFIG);
        esp_coex_status_bit_clear(ESP_COEX_ST_TYPE_BLE, ESP_COEX_BLE_ST_MESH_TRAFFIC);
        esp_coex_status_bit_clear(ESP_COEX_ST_TYPE_BLE, ESP_COEX_BLE_ST_MESH_STANDBY);
        esp_coex_status_bit_set(ESP_COEX_ST_TYPE_BLE, ESP_COEX_BLE_ST_MESH_TRAFFIC);

        ESP_LOGW(TAG, "BLE Mesh enters Traffic mode");

        example_ble_mesh_test_init();
        example_ble_mesh_send_test_msg(false);
    }
}

static void board_button_init(void)
{
    button_handle_t btn_handle = iot_button_create(BUTTON_IO_NUM, BUTTON_ACTIVE_LEVEL);
    if (btn_handle) {
        iot_button_set_evt_cb(btn_handle, BUTTON_CB_RELEASE, button_tap_cb, "RELEASE");
    }
}

void board_init(void)
{
    board_led_init();
    board_button_init();
}
