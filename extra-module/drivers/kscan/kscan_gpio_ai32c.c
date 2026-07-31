/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: MIT
 *
 * AI32C 3-channel capacitive touch kscan driver.
 *
 * AI32C outputs a 2-bit binary code on OUT1/OUT2 (active low):
 *   OUT2:OUT1 = 00 → KEY1 (zone 1)
 *   OUT2:OUT1 = 01 → KEY2 (zone 2)
 *   OUT2:OUT1 = 10 → KEY3 (zone 3)
 *   OUT2:OUT1 = 11 → no touch
 *
 * This driver polls both GPIOs, decodes the active key, and emits
 * press/release events. AI32C has built-in debounce so no software
 * debouncing is needed.
 */

#define DT_DRV_COMPAT zmk_kscan_gpio_ai32c

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/kscan.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct kscan_ai32c_config {
    struct gpio_dt_spec out1;
    struct gpio_dt_spec out2;
    int32_t poll_period_ms;
};

struct kscan_ai32c_data {
    const struct device *dev;
    kscan_callback_t callback;
    struct k_work_delayable work;
    int8_t prev_key; /* Previously active key index, -1 = none */
};

/**
 * Decode the 2-bit AI32C output to a key index.
 *
 * @param out1  GPIO reading of OUT1 (0 = LOW/active, 1 = HIGH/inactive)
 * @param out2  GPIO reading of OUT2 (0 = LOW/active, 1 = HIGH/inactive)
 * @return      Key index 0/1/2 for KEY1/KEY2/KEY3, or -1 for no touch
 */
static int8_t ai32c_decode(int out1, int out2) {
    switch ((out2 << 1) | out1) {
    case 0:
        return 0; /* both LOW = KEY1 */
    case 1:
        return 1; /* OUT1 HIGH, OUT2 LOW = KEY2 */
    case 2:
        return 2; /* OUT1 LOW, OUT2 HIGH = KEY3 */
    default:
        return -1; /* both HIGH = no touch */
    }
}

static void kscan_ai32c_work_handler(struct k_work *work) {
    struct k_work_delayable *dwork = CONTAINER_OF(work, struct k_work_delayable, work);
    struct kscan_ai32c_data *data = CONTAINER_OF(dwork, struct kscan_ai32c_data, work);
    const struct kscan_ai32c_config *cfg = data->dev->config;

    int out1 = gpio_pin_get_dt(&cfg->out1);
    int out2 = gpio_pin_get_dt(&cfg->out2);

    if (out1 < 0 || out2 < 0) {
        LOG_ERR("Failed to read AI32C GPIOs (out1=%d, out2=%d)", out1, out2);
        goto reschedule;
    }

    int8_t active_key = ai32c_decode(out1, out2);

    if (active_key != data->prev_key) {
        /* Release previous key */
        if (data->prev_key >= 0) {
            LOG_DBG("Touch release: key %d", data->prev_key);
            data->callback(data->dev, data->prev_key, 0, false);
        }
        /* Press new key */
        if (active_key >= 0) {
            LOG_DBG("Touch press:   key %d", active_key);
            data->callback(data->dev, active_key, 0, true);
        }
        data->prev_key = active_key;
    }

reschedule:
    k_work_reschedule(&data->work, K_MSEC(cfg->poll_period_ms));
}

static int kscan_ai32c_configure(const struct device *dev, kscan_callback_t callback) {
    struct kscan_ai32c_data *data = dev->data;

    if (!callback) {
        return -EINVAL;
    }

    data->callback = callback;
    return 0;
}

static int kscan_ai32c_enable(const struct device *dev) {
    struct kscan_ai32c_data *data = dev->data;
    const struct kscan_ai32c_config *cfg = dev->config;

    data->prev_key = -1;
    k_work_reschedule(&data->work, K_MSEC(cfg->poll_period_ms));

    LOG_DBG("AI32C kscan enabled, poll period %d ms", cfg->poll_period_ms);
    return 0;
}

static int kscan_ai32c_disable(const struct device *dev) {
    struct kscan_ai32c_data *data = dev->data;

    k_work_cancel_delayable(&data->work);
    data->prev_key = -1;

    return 0;
}

static int kscan_ai32c_init(const struct device *dev) {
    struct kscan_ai32c_data *data = dev->data;
    const struct kscan_ai32c_config *cfg = dev->config;

    data->dev = dev;

    if (!device_is_ready(cfg->out1.port)) {
        LOG_ERR("AI32C OUT1 GPIO port not ready");
        return -ENODEV;
    }
    if (!device_is_ready(cfg->out2.port)) {
        LOG_ERR("AI32C OUT2 GPIO port not ready");
        return -ENODEV;
    }

    int err = gpio_pin_configure_dt(&cfg->out1, GPIO_INPUT);
    if (err) {
        LOG_ERR("Failed to configure OUT1 GPIO (err=%d)", err);
        return err;
    }
    err = gpio_pin_configure_dt(&cfg->out2, GPIO_INPUT);
    if (err) {
        LOG_ERR("Failed to configure OUT2 GPIO (err=%d)", err);
        return err;
    }

    k_work_init_delayable(&data->work, kscan_ai32c_work_handler);

    LOG_DBG("AI32C kscan initialized: OUT1=%s pin=%d, OUT2=%s pin=%d",
            cfg->out1.port->name, cfg->out1.pin, cfg->out2.port->name, cfg->out2.pin);

    return 0;
}

static const struct kscan_driver_api kscan_ai32c_api = {
    .config = kscan_ai32c_configure,
    .enable_callback = kscan_ai32c_enable,
    .disable_callback = kscan_ai32c_disable,
};

#define KSCAN_AI32C_INIT(n)                                                                        \
    static const struct kscan_ai32c_config kscan_ai32c_config_##n = {                              \
        .out1 = GPIO_DT_SPEC_INST_GET_BY_IDX(n, out_gpios, 0),                                    \
        .out2 = GPIO_DT_SPEC_INST_GET_BY_IDX(n, out_gpios, 1),                                    \
        .poll_period_ms = DT_INST_PROP(n, poll_period_ms),                                         \
    };                                                                                             \
    static struct kscan_ai32c_data kscan_ai32c_data_##n;                                           \
    DEVICE_DT_INST_DEFINE(n, kscan_ai32c_init, NULL, &kscan_ai32c_data_##n,                        \
                          &kscan_ai32c_config_##n, POST_KERNEL, CONFIG_KSCAN_INIT_PRIORITY,        \
                          &kscan_ai32c_api);

DT_INST_FOREACH_STATUS_OKAY(KSCAN_AI32C_INIT)
