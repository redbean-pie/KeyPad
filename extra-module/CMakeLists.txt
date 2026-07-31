# Copyright (c) 2026
# SPDX-License-Identifier: MIT

add_subdirectory_ifdef(CONFIG_ZMK_KSCAN_GPIO_AI32C drivers/kscan)
add_subdirectory_ifdef(CONFIG_ZMK_BATTERY_LED battery_led)
add_subdirectory_ifdef(CONFIG_ZMK_BLE_LED ble_led)
