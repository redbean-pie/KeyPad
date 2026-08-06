/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: MIT
 *
 * AI32C 触摸键 kscan 驱动（poll 版，三键独立）。
 * AI32C OUT 是 push-pull 输出，2-wire 编码：
 *   11=空闲  10=KEY3  01=KEY2  00=KEY1
 * 按编码报不同 col：KEY1->col0  KEY2->col1  KEY3->col2
 * 数据手册：同时多键按 KEY1>KEY2>KEY3 优先级，无需特殊处理。
 * poll 周期 10ms，ZMK 去抖层处理抖动，无需 PULL_UP。
 */

#define DT_DRV_COMPAT zmk_kscan_gpio_ai32c

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/kscan.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#define AI32C_POLL_MS 10

struct kscan_ai32c_config {
    struct gpio_dt_spec out1;
    struct gpio_dt_spec out2;
};

/* 无按键返回 -1，否则返回 col（0/1/2） */
static int ai32c_read_key(const struct kscan_ai32c_config *cfg) {
    int o1 = gpio_pin_get_raw(cfg->out1.port, cfg->out1.pin);
    int o2 = gpio_pin_get_raw(cfg->out2.port, cfg->out2.pin);
    /* 编码表：00=KEY1  01=KEY2  10=KEY3  11=空闲 */
    if (o1 == 0 && o2 == 0) return 0;
    if (o1 == 1 && o2 == 0) return 1;
    if (o1 == 0 && o2 == 1) return 2;
    return -1;
}

struct kscan_ai32c_data {
    const struct device *dev;
    kscan_callback_t callback;
    struct k_work_delayable poll_work;
    int active_col; /* 当前按下的 col，-1=无 */
};

static void ai32c_poll_handler(struct k_work *work) {
    struct k_work_delayable *dwork = k_work_delayable_from_work(work);
    struct kscan_ai32c_data *data = CONTAINER_OF(dwork, struct kscan_ai32c_data, poll_work);
    const struct device *dev = data->dev;
    const struct kscan_ai32c_config *cfg = dev->config;

    int now = ai32c_read_key(cfg);

    if (now != data->active_col) {
        /* 先释放旧键 */
        if (data->active_col >= 0 && data->callback) {
            data->callback(dev, 0, data->active_col, false);
        }
        /* 再按下新键 */
        if (now >= 0 && data->callback) {
            data->callback(dev, 0, now, true);
        }
        data->active_col = now;
        LOG_INF("AI32C touch %s col=%d", now >= 0 ? "press" : "release", now);
    }

    k_work_schedule(&data->poll_work, K_MSEC(AI32C_POLL_MS));
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
    k_work_schedule(&data->poll_work, K_MSEC(AI32C_POLL_MS));
    return 0;
}

static int ai32c_disable(const struct device *dev) {
    struct kscan_ai32c_data *data = dev->data;
    k_work_cancel_delayable(&data->poll_work);
    return 0;
}

static int ai32c_init(const struct device *dev) {
    struct kscan_ai32c_data *data = dev->data;
    const struct kscan_ai32c_config *cfg = dev->config;
    int err;

    data->dev = dev;
    data->active_col = -1;

    if (!device_is_ready(cfg->out1.port) || !device_is_ready(cfg->out2.port)) {
        LOG_ERR("AI32C GPIO port not ready");
        return -ENODEV;
    }

    /* AI32C OUT 是 push-pull 输出，主动驱动高低电平，不需要 PULL_UP。
     * 用 gpio_pin_configure 忽略 dt_flags，强制纯输入，避免 PULL_UP 与
     * 外部驱动（尤其飞线 5V 时）冲突。 */
    err = gpio_pin_configure(cfg->out1.port, cfg->out1.pin, GPIO_INPUT);
    if (err) {
        LOG_ERR("OUT1 configure failed: %d", err);
        return err;
    }
    err = gpio_pin_configure(cfg->out2.port, cfg->out2.pin, GPIO_INPUT);
    if (err) {
        LOG_ERR("OUT2 configure failed: %d", err);
        return err;
    }

    k_work_init_delayable(&data->poll_work, ai32c_poll_handler);

    /* init 即启动 poll，不依赖 enable_callback（ZMK composite 不调用 enable） */
    k_work_schedule(&data->poll_work, K_MSEC(AI32C_POLL_MS));

    LOG_INF("AI32C kscan init: poll %dms, OUT1=%s pin %d, OUT2=%s pin %d",
            AI32C_POLL_MS,
            cfg->out1.port->name, cfg->out1.pin,
            cfg->out2.port->name, cfg->out2.pin);
    return 0;
}

static const struct kscan_driver_api ai32c_api = {
    .config = ai32c_configure,
    .enable_callback = ai32c_enable,
    .disable_callback = ai32c_disable,
};

#define KSCAN_AI32C_INIT(n)                                                                        \
    static const struct kscan_ai32c_config ai32c_config_##n = {                                    \
        .out1 = GPIO_DT_SPEC_INST_GET_BY_IDX(n, out_gpios, 0),                                     \
        .out2 = GPIO_DT_SPEC_INST_GET_BY_IDX(n, out_gpios, 1),                                     \
    };                                                                                             \
    static struct kscan_ai32c_data ai32c_data_##n;                                                 \
    DEVICE_DT_INST_DEFINE(n, ai32c_init, NULL, &ai32c_data_##n, &ai32c_config_##n,                \
                          POST_KERNEL, CONFIG_KSCAN_INIT_PRIORITY, &ai32c_api);

DT_INST_FOREACH_STATUS_OKAY(KSCAN_AI32C_INIT)
