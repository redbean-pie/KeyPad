/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: MIT
 *
 * Battery level indicator using an independent WS2812 chain (led_strip_batt).
 * Avoids conflict with rgb_underglow (which owns the main strip).
 *
 * Trigger: entering layer 1 (fn layer) shows battery for 2 seconds.
 * Mapping (2 LEDs):
 *   soc > 75 : both green
 *   soc > 50 : 1 green
 *   soc > 25 : both red
 *   soc > 10 : 1 red
 *   soc <= 10: both red, blinking 500ms
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zmk/battery.h>
#include <zmk/event_manager.h>
#include <zmk/events/layer_state_changed.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#define BATT_STRIP DT_NODELABEL(led_strip_batt)
#define BATT_LED_COUNT 2
#define BATT_LAYER 1
#define BATT_SHOW_MS K_MSEC(2000)
#define BATT_BLINK_MS K_MSEC(500)
#define BATT_BRIGHTNESS 30

static const struct device *strip;
static struct k_work_delayable blink_work;
static struct k_work_delayable hide_work;
static bool blink_on;

static void batt_led_set(uint8_t n_green, uint8_t n_red) {
    struct led_rgb px[BATT_LED_COUNT] = {0};

    for (int i = 0; i < BATT_LED_COUNT; i++) {
        if (i < n_green) {
            px[i].g = BATT_BRIGHTNESS;
        } else if (i < n_green + n_red) {
            px[i].r = BATT_BRIGHTNESS;
        }
    }

    int err = led_strip_update_rgb(strip, px, BATT_LED_COUNT);
    if (err) {
        LOG_ERR("Battery LED update failed (%d)", err);
    }
}

static void batt_led_clear(void) {
    batt_led_set(0, 0);
}

static void batt_blink_handler(struct k_work *work) {
    blink_on = !blink_on;

    struct led_rgb px[BATT_LED_COUNT] = {0};
    if (blink_on) {
        px[0].r = BATT_BRIGHTNESS;
        px[1].r = BATT_BRIGHTNESS;
    }

    int err = led_strip_update_rgb(strip, px, BATT_LED_COUNT);
    if (err) {
        LOG_ERR("Battery LED blink update failed (%d)", err);
    }

    k_work_reschedule(&blink_work, BATT_BLINK_MS);
}

static void batt_hide_handler(struct k_work *work) {
    k_work_cancel_delayable(&blink_work);
    batt_led_clear();
}

static void batt_show(void) {
    uint8_t soc = zmk_battery_state_of_charge();

    k_work_cancel_delayable(&blink_work);
    k_work_cancel_delayable(&hide_work);

    if (soc <= 10) {
        blink_on = true;
        k_work_reschedule(&blink_work, K_NO_WAIT);
    } else if (soc <= 25) {
        batt_led_set(0, 1); /* 1 red */
    } else if (soc <= 50) {
        batt_led_set(0, 2); /* 2 red */
    } else if (soc <= 75) {
        batt_led_set(1, 0); /* 1 green */
    } else {
        batt_led_set(2, 0); /* 2 green */
    }

    k_work_reschedule(&hide_work, BATT_SHOW_MS);
}

static int batt_led_listener(const zmk_event_t *eh) {
    const struct zmk_layer_state_changed *ev = as_zmk_layer_state_changed(eh);
    if (ev == NULL) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    if (ev->layer == BATT_LAYER) {
        if (ev->state) {
            LOG_DBG("fn layer active, showing battery (%d%%)", zmk_battery_state_of_charge());
            batt_show();
        } else {
            k_work_cancel_delayable(&blink_work);
            k_work_cancel_delayable(&hide_work);
            batt_led_clear();
        }
    }

    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(batt_led, batt_led_listener);
ZMK_SUBSCRIPTION(batt_led, zmk_layer_state_changed);

static int batt_led_init(void) {
    strip = DEVICE_DT_GET(BATT_STRIP);
    if (!device_is_ready(strip)) {
        LOG_ERR("Battery LED strip not ready");
        return -ENODEV;
    }

    k_work_init_delayable(&blink_work, batt_blink_handler);
    k_work_init_delayable(&hide_work, batt_hide_handler);

    batt_led_clear();

    LOG_INF("Battery LED indicator initialized");
    return 0;
}

SYS_INIT(batt_led_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
