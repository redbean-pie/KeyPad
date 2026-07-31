/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: MIT
 *
 * Battery level / charging indicator using an independent WS2812 chain
 * (led_strip_batt). Avoids conflict with rgb_underglow (which owns the
 * main strip).
 *
 * Display priority:
 *  1. USB powered (charging): sustained indication
 *       - soc >= 100 : both green (full)
 *       - otherwise   : both orange, blinking (charging)
 *  2. Layer 1 (fn layer) press: show battery for 2 seconds
 *       - soc > 75 : both green / >50 : 1 green / >25 : both red
 *       - soc > 10 : 1 red / <=10 : both red, blinking (low battery)
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
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/usb.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#define BATT_STRIP DT_NODELABEL(led_strip_batt)
#define BATT_LED_COUNT 2
#define BATT_LAYER 1
#define BATT_SHOW_MS K_MSEC(2000)
#define BATT_BLINK_MS K_MSEC(500)
#define BATT_BRIGHTNESS 30

enum blink_color { BLINK_RED, BLINK_ORANGE };

static const struct device *strip;
static struct k_work_delayable blink_work;
static struct k_work_delayable hide_work;
static bool blink_on;
static enum blink_color blink_color;
static bool charging;

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
        for (int i = 0; i < BATT_LED_COUNT; i++) {
            px[i].r = BATT_BRIGHTNESS;
            if (blink_color == BLINK_ORANGE) {
                px[i].g = BATT_BRIGHTNESS / 2;
            }
        }
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
        blink_color = BLINK_RED;
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

static void charging_show(void) {
    charging = true;
    k_work_cancel_delayable(&hide_work);

    uint8_t soc = zmk_battery_state_of_charge();

    if (soc >= 100) {
        k_work_cancel_delayable(&blink_work);
        batt_led_set(2, 0); /* full: 2 green steady */
    } else {
        blink_color = BLINK_ORANGE;
        blink_on = true;
        k_work_reschedule(&blink_work, K_NO_WAIT); /* charging: orange blink */
    }
}

static void charging_clear(void) {
    charging = false;
    k_work_cancel_delayable(&blink_work);
    k_work_cancel_delayable(&hide_work);
    batt_led_clear();
}

static int batt_led_listener(const zmk_event_t *eh) {
    const struct zmk_layer_state_changed *ev = as_zmk_layer_state_changed(eh);
    if (ev == NULL) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    /* Charging indication has priority over the fn-layer battery peek. */
    if (charging) {
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

static int usb_conn_listener(const zmk_event_t *eh) {
    const struct zmk_usb_conn_state_changed *ev = as_zmk_usb_conn_state_changed(eh);
    if (ev == NULL) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    if (ev->conn_state != ZMK_USB_CONN_NONE) {
        LOG_DBG("USB powered, showing charging state");
        charging_show();
    } else {
        charging_clear();
    }

    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(batt_usb, usb_conn_listener);
ZMK_SUBSCRIPTION(batt_usb, zmk_usb_conn_state_changed);

static int batt_led_init(void) {
    strip = DEVICE_DT_GET(BATT_STRIP);
    if (!device_is_ready(strip)) {
        LOG_ERR("Battery LED strip not ready");
        return -ENODEV;
    }

    k_work_init_delayable(&blink_work, batt_blink_handler);
    k_work_init_delayable(&hide_work, batt_hide_handler);

    batt_led_clear();

    /* If already plugged into USB at boot, show charging immediately. */
    if (zmk_usb_is_powered()) {
        charging_show();
    }

    LOG_INF("Battery LED indicator initialized");
    return 0;
}

SYS_INIT(batt_led_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
