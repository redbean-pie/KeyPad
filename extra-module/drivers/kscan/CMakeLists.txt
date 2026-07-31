# Copyright (c) 2026
# SPDX-License-Identifier: MIT

zephyr_library_amend()

zephyr_library_sources_ifdef(CONFIG_ZMK_KSCAN_GPIO_AI32C kscan_gpio_ai32c.c)
