# Copyright (c) 2026
# SPDX-License-Identifier: MIT

zephyr_library()

zephyr_library_include_directories(
    ${CMAKE_CURRENT_LIST_DIR}/../../zmk/app/include
    ${CMAKE_CURRENT_LIST_DIR}/../../zmk/app/module/include
)

zephyr_library_sources_ifdef(CONFIG_ZMK_BATTERY_LED battery_led.c)
