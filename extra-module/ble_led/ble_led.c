/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: MIT
 *
 * Bluetooth connection status LED (single-color, GPIO direct drive).
 *
 * LED behavior (D21, active high):
 *   - Active profile connected   : LED solid on
 *   - Not connected / searching  : LED blinking (500ms)
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zmk/ble.h>
#include <zmk/event_manager.h>
#include <zmk/events/ble_active_profile_changed.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#define BLE_LED_NODE DT_NODELABEL(ble_status_led)

static const struct gpio_dt_spec ble_led_gpio = GPIO_DT_SPEC_GET(BLE_LED_NODE, gpios);
static struct k_work_delayable blink_work;

static void ble_led_set(bool on) {
    int err = gpio_pin_set_dt(&ble_led_gpio, on ? 1 : 0);
    if (err) {
        LOG_ERR("Failed to set BLE LED (%d)", err);
    }
}

static void ble_led_blink_handler(struct k_work *work) {
    static bool on = false;

    on = !on;
    ble_led_set(on);

    k_work_reschedule(&blink_work, K_MSEC(500));
}

static void ble_led_show_connected(void) {
    k_work_cancel_delayable(&blink_work);
    ble_led_set(true); /* solid on */
}

static void ble_led_show_disconnected(void) {
    k_work_reschedule(&blink_work, K_NO_WAIT); /* blink */
}

static int ble_led_listener(const zmk_event_t *eh) {
    const struct zmk_ble_active_profile_changed *ev = as_zmk_ble_active_profile_changed(eh);
    if (ev == NULL) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    if (zmk_ble_active_profile_is_connected()) {
        LOG_DBG("BLE connected, LED on");
        ble_led_show_connected();
    } else {
        LOG_DBG("BLE disconnected, LED blinking");
        ble_led_show_disconnected();
    }

    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(ble_led, ble_led_listener);
ZMK_SUBSCRIPTION(ble_led, zmk_ble_active_profile_changed);

static int ble_led_init(void) {
    if (!gpio_is_ready_dt(&ble_led_gpio)) {
        LOG_ERR("BLE LED GPIO port not ready");
        return -ENODEV;
    }

    int err = gpio_pin_configure_dt(&ble_led_gpio, GPIO_OUTPUT_INACTIVE);
    if (err) {
        LOG_ERR("Failed to configure BLE LED GPIO (err=%d)", err);
        return err;
    }

    k_work_init_delayable(&blink_work, ble_led_blink_handler);

    /* Set initial state based on current connection. */
    if (zmk_ble_active_profile_is_connected()) {
        ble_led_show_connected();
    } else {
        ble_led_show_disconnected();
    }

    LOG_INF("BLE LED initialized");
    return 0;
}

SYS_INIT(ble_led_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
