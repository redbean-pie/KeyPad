/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: MIT
 *
 * AI32C 触摸键 kscan 驱动（poll 版，三键独立）。
 * AI32C OUT 是 push-pull 输出，2-wire 编码。
 *
 * 编码表（数据手册真值表按 (OUT2,OUT1) 给出，此处按 (OUT1,OUT2) 排列，
 * 与下方 o1/o2 变量一一对应）：
 *   (OUT1,OUT2) 11=空闲  10=KEY2  01=KEY3  00=KEY1
 * 按编码报不同 col：KEY1->col0  KEY2->col1  KEY3->col2
 * 数据手册：同时多键按 KEY1>KEY2>KEY3 优先级，无需特殊处理。
 * poll 周期 10ms，ZMK 去抖层处理抖动，无需 PULL_UP。
 *
 * 深睡眠（CONFIG_ZMK_SLEEP）触摸唤醒：
 * ZMK 深睡眠走 sys_poweroff()（nRF52 System Off），唤醒靠 GPIO SENSE。
 * 本驱动平时纯轮询无中断，因此监听 zmk_activity_state_changed：
 * 进入 SLEEP 前停轮询、给两根 OUT 挂 GPIO_INT_LEVEL_LOW（nRF 走
 * PORT/SENSE 低功耗路径，System Off 下仍有效）。空闲时 OUT=11（高），
 * 任一触摸拉低即触发 DETECT 唤醒（唤醒=复位重启，开机后轮询读回按住状态）。
 */

#define DT_DRV_COMPAT zmk_kscan_gpio_ai32c

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/kscan.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zmk/activity.h>
#include <zmk/event_manager.h>
#include <zmk/events/activity_state_changed.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#define AI32C_POLL_MS 10
/* AI32C 上电自校准 T_init=400ms，期间 OUT 未定义；首个 poll 延后避开误报 */
#define AI32C_INIT_DELAY_MS 500

struct kscan_ai32c_config {
    struct gpio_dt_spec out1;
    struct gpio_dt_spec out2;
};

/* 无按键返回 -1，否则返回 col（0/1/2） */
static int ai32c_read_key(const struct kscan_ai32c_config *cfg) {
    int o1 = gpio_pin_get_raw(cfg->out1.port, cfg->out1.pin);
    int o2 = gpio_pin_get_raw(cfg->out2.port, cfg->out2.pin);
    /* 编码表（按 OUT1,OUT2 顺序）：00=KEY1  10=KEY2  01=KEY3  11=空闲 */
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
    bool wake_armed; /* 深睡眠前是否已挂唤醒中断 */
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

/* 深睡眠唤醒中断：空闲 OUT=11（高），任一触摸拉低触发。
 * 不注册回调——只为让 nRF SENSE/DETECT 在 System Off 下保持有效。 */
static int ai32c_arm_wake(struct kscan_ai32c_data *data, const struct kscan_ai32c_config *cfg) {
    int err = gpio_pin_interrupt_configure(cfg->out1.port, cfg->out1.pin, GPIO_INT_LEVEL_LOW);
    if (err) {
        LOG_ERR("AI32C OUT1 wake configure failed: %d", err);
        return err;
    }
    err = gpio_pin_interrupt_configure(cfg->out2.port, cfg->out2.pin, GPIO_INT_LEVEL_LOW);
    if (err) {
        LOG_ERR("AI32C OUT2 wake configure failed: %d", err);
        return err;
    }
    data->wake_armed = true;
    LOG_INF("AI32C sleep: wake interrupts armed");
    return 0;
}

static void ai32c_disarm_wake(struct kscan_ai32c_data *data,
                              const struct kscan_ai32c_config *cfg) {
    gpio_pin_interrupt_configure(cfg->out1.port, cfg->out1.pin, GPIO_INT_DISABLE);
    gpio_pin_interrupt_configure(cfg->out2.port, cfg->out2.pin, GPIO_INT_DISABLE);
    data->wake_armed = false;
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

    /* init 即启动 poll，不依赖 enable_callback（ZMK composite 不调用 enable）。
     * 首个 poll 延迟 500ms：避开 AI32C 上电 400ms 自校准期未定义输出。 */
    k_work_schedule(&data->poll_work, K_MSEC(AI32C_INIT_DELAY_MS));

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

/* 监听 ZMK 活动状态：
 * - SLEEP（即将 sys_poweroff）：停轮询，挂 SENSE 唤醒中断
 * - ACTIVE（从睡眠异常路径恢复）：解除中断，恢复轮询 */
static int ai32c_activity_listener(const zmk_event_t *eh) {
    const struct zmk_activity_state_changed *ev = as_zmk_activity_state_changed(eh);
    if (ev == NULL) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    /* 本驱动仅实例 0；无节点时该文件不会被编译 */
    const struct device *dev = DEVICE_DT_INST_GET(0);
    struct kscan_ai32c_data *data = dev->data;

    if (ev->state == ZMK_ACTIVITY_SLEEP) {
        k_work_cancel_delayable(&data->poll_work);
        ai32c_arm_wake(data, dev->config);
    } else if (ev->state == ZMK_ACTIVITY_ACTIVE && data->wake_armed) {
        ai32c_disarm_wake(data, dev->config);
        k_work_schedule(&data->poll_work, K_MSEC(AI32C_POLL_MS));
    }

    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(ai32c_sleep, ai32c_activity_listener);
ZMK_SUBSCRIPTION(ai32c_sleep, zmk_activity_state_changed);
