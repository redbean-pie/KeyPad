/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: MIT
 *
 * AI32C 3-channel capacitive touch kscan driver (interrupt + poll hybrid).
 *
 * AI32C outputs a 2-bit binary code on OUT1/OUT2 (active low):
 *   OUT2:OUT1 = 00 → KEY1 (zone 1)
 *   OUT2:OUT1 = 01 → KEY2 (zone 2)
 *   OUT2:OUT1 = 10 → KEY3 (zone 3)
 *   OUT2:OUT1 = 11 → no touch
 *
 * Power model:
 * - No touch: both OUTs idle high; GPIO level interrupts wait on them.
 *   No polling runs, so the MCU stays idle (low power, no wake-up spam).
 * - Touch: an OUT goes low → IRQ → work reads/decode/reports the key,
 *   then keeps polling (poll-period-ms) while any key is held to detect
 *   release. On release it re-arms the level interrupts.
 * - PM: the device declares PM support so ZMK can wakeup-enable it; in
 *   deep sleep the touch (OUT level change) wakes the MCU.
 */

#define DT_DRV_COMPAT zmk_kscan_gpio_ai32c

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/kscan.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/util.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct kscan_ai32c_config {
    struct gpio_dt_spec out1;
    struct gpio_dt_spec out2;
    int32_t poll_period_ms;
};

struct kscan_ai32c_irq_callback {
    const struct device *dev;
    struct gpio_callback callback;
};

struct kscan_ai32c_data {
    const struct device *dev;
    kscan_callback_t callback;
    struct k_work_delayable work;
    struct kscan_ai32c_irq_callback irq_out1;
    struct kscan_ai32c_irq_callback irq_out2;
    int8_t prev_key; /* -1 = no touch */
};

static int ai32c_interrupt_configure(const struct device *dev, gpio_flags_t flags) {
    const struct kscan_ai32c_config *cfg = dev->config;

    int err = gpio_pin_interrupt_configure_dt(&cfg->out1, flags);
    if (err) {
        LOG_ERR("Failed to configure OUT1 interrupt (err=%d)", err);
        return err;
    }
    err = gpio_pin_interrupt_configure_dt(&cfg->out2, flags);
    if (err) {
        LOG_ERR("Failed to configure OUT2 interrupt (err=%d)", err);
        return err;
    }

    return 0;
}

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

static void ai32c_irq_handler(const struct device *port, struct gpio_callback *cb,
                              const gpio_port_pins_t pin) {
    struct kscan_ai32c_irq_callback *irq =
        CONTAINER_OF(cb, struct kscan_ai32c_irq_callback, callback);
    const struct device *dev = irq->dev;
    struct kscan_ai32c_data *data = dev->data;

    /* Disable interrupts to avoid re-entry while we read. */
    ai32c_interrupt_configure(dev, GPIO_INT_DISABLE);

    k_work_reschedule(&data->work, K_NO_WAIT);
}

static void ai32c_read_continue(const struct device *dev) {
    const struct kscan_ai32c_config *cfg = dev->config;
    struct kscan_ai32c_data *data = dev->data;

    /* A key is still held; keep polling to detect the release. */
    k_work_reschedule(&data->work, K_MSEC(cfg->poll_period_ms));
}

static void ai32c_read_end(const struct device *dev) {
    struct kscan_ai32c_data *data = dev->data;

    data->prev_key = -1;

    /* All keys released, return to interrupt-wait. */
    ai32c_interrupt_configure(dev, GPIO_INT_LEVEL_ACTIVE);
}

static void ai32c_work_handler(struct k_work *work) {
    struct k_work_delayable *dwork = CONTAINER_OF(work, struct k_work_delayable, work);
    struct kscan_ai32c_data *data = CONTAINER_OF(dwork, struct kscan_ai32c_data, work);
    const struct device *dev = data->dev;
    const struct kscan_ai32c_config *cfg = dev->config;

    int out1 = gpio_pin_get_dt(&cfg->out1);
    int out2 = gpio_pin_get_dt(&cfg->out2);

    if (out1 < 0 || out2 < 0) {
        LOG_ERR("Failed to read AI32C GPIOs (out1=%d, out2=%d)", out1, out2);
        ai32c_read_end(dev);
        return;
    }

    int8_t active_key = ai32c_decode(out1, out2);

    if (active_key != data->prev_key) {
        if (data->prev_key >= 0) {
            LOG_DBG("Touch release: key %d", data->prev_key);
            data->callback(dev, data->prev_key, 0, false);
        }
        if (active_key >= 0) {
            LOG_DBG("Touch press:   key %d", active_key);
            data->callback(dev, active_key, 0, true);
        }
        data->prev_key = active_key;
    }

    if (active_key >= 0) {
        ai32c_read_continue(dev);
    } else {
        ai32c_read_end(dev);
    }
}

static int ai32c_configure(const struct device *dev, kscan_callback_t callback) {
    struct kscan_ai32c_data *data = dev->data;

    if (!callback) {
        return -EINVAL;
    }

    data->callback = callback;
    return 0;
}

static int ai32c_enable(const struct device *dev) {
    struct kscan_ai32c_data *data = dev->data;

    data->prev_key = -1;
    return ai32c_interrupt_configure(dev, GPIO_INT_LEVEL_ACTIVE);
}

static int ai32c_disable(const struct device *dev) {
    struct kscan_ai32c_data *data = dev->data;

    k_work_cancel_delayable(&data->work);
    data->prev_key = -1;

    return ai32c_interrupt_configure(dev, GPIO_INT_DISABLE);
}

static int ai32c_init(const struct device *dev) {
    struct kscan_ai32c_data *data = dev->data;
    const struct kscan_ai32c_config *cfg = dev->config;

    data->dev = dev;
    data->prev_key = -1;

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

    data->irq_out1.dev = dev;
    gpio_init_callback(&data->irq_out1.callback, ai32c_irq_handler, BIT(cfg->out1.pin));
    err = gpio_add_callback(cfg->out1.port, &data->irq_out1.callback);
    if (err) {
        LOG_ERR("Failed to add OUT1 callback (err=%d)", err);
        return err;
    }

    data->irq_out2.dev = dev;
    gpio_init_callback(&data->irq_out2.callback, ai32c_irq_handler, BIT(cfg->out2.pin));
    err = gpio_add_callback(cfg->out2.port, &data->irq_out2.callback);
    if (err) {
        LOG_ERR("Failed to add OUT2 callback (err=%d)", err);
        return err;
    }

    k_work_init_delayable(&data->work, ai32c_work_handler);

    LOG_DBG("AI32C kscan initialized: OUT1=%s pin=%d, OUT2=%s pin=%d", cfg->out1.port->name,
            cfg->out1.pin, cfg->out2.port->name, cfg->out2.pin);

    return 0;
}

static const struct kscan_driver_api ai32c_api = {
    .config = ai32c_configure,
    .enable_callback = ai32c_enable,
    .disable_callback = ai32c_disable,
};

#if IS_ENABLED(CONFIG_PM_DEVICE)

static int ai32c_pm_action(const struct device *dev, enum pm_device_action action) {
    switch (action) {
    case PM_DEVICE_ACTION_SUSPEND:
        return ai32c_disable(dev);
    case PM_DEVICE_ACTION_RESUME:
        return ai32c_enable(dev);
    default:
        return -ENOTSUP;
    }
}

#endif /* IS_ENABLED(CONFIG_PM_DEVICE) */

#define KSCAN_AI32C_INIT(n)                                                                        \
    static const struct kscan_ai32c_config ai32c_config_##n = {                                    \
        .out1 = GPIO_DT_SPEC_INST_GET_BY_IDX(n, out_gpios, 0),                                     \
        .out2 = GPIO_DT_SPEC_INST_GET_BY_IDX(n, out_gpios, 1),                                     \
        .poll_period_ms = DT_INST_PROP(n, poll_period_ms),                                          \
    };                                                                                             \
    static struct kscan_ai32c_data ai32c_data_##n;                                                 \
    PM_DEVICE_DT_INST_DEFINE(n, ai32c_pm_action);                                                  \
    DEVICE_DT_INST_DEFINE(n, ai32c_init, PM_DEVICE_DT_INST_GET(n), &ai32c_data_##n,                \
                          &ai32c_config_##n, POST_KERNEL, CONFIG_KSCAN_INIT_PRIORITY,              \
                          &ai32c_api);

DT_INST_FOREACH_STATUS_OKAY(KSCAN_AI32C_INIT)
