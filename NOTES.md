# 项目备注

## 项目概况
- **类型**：ZMK 数字小键盘固件
- **板子**：nice!nano v2（nRF52840，Pro Micro 兼容接口）
- **Shield**：numpad（5×4 矩阵 + EC11 旋钮 + SSD1306 OLED 0.91" 128×32）
- **当前状态**：固件编译通过 ✅

## 引脚分配

| 功能 | Pro Micro | nRF52840 |
|------|-----------|----------|
| 矩阵行 R0 | D20 / A2 | P0.29 |
| 矩阵行 R1 | D16 | P0.10 |
| 矩阵行 R2 | D14 | P1.11 |
| 矩阵行 R3 | D15 | P1.13 |
| 矩阵行 R4 | D18 / A0 | P1.15 |
| 矩阵列 C0 | D9 / A9 | P1.6 |
| 矩阵列 C1 | D6 / A7 | P1.0 |
| 矩阵列 C2 | D5 | P0.24 |
| 矩阵列 C3 | D4 / A6 | P0.22 |
| EC11 A 相 | D7 | P0.11 |
| EC11 B 相 | D8 / A8 | P1.4 |
| I2C SDA（OLED） | D2 | P0.17 |
| I2C SCL（OLED） | D3 | P0.20 |
| UART RX（空闲） | D0 | P0.8 |
| UART TX（空闲） | D1 | P0.6 |

**未使用引脚**：D10/A10、D19/A1、D21/A3 可作扩展。

## 构建方法

### 本地工具链（推荐）
```powershell
west build -s zmk/app -b nice_nano//zmk -d build -- -C zmk-cache.cmake -DSHIELD=numpad
```

增量编译：
```powershell
west build -d build
```

> **注意**：`zmk-cache.cmake` 不入库（已 gitignore），需本机自行创建：
> ```cmake
> set(ZMK_EXTRA_MODULES "C:/Users/as176/Desktop/Code/TRAE/KEYPAD/extra-module" CACHE STRING "")
> set(ZEPHYR_TOOLCHAIN_VARIANT "gnuarmemb" CACHE STRING "")
> set(GNUARMEMB_TOOLCHAIN_PATH "C:/ProgramData/chocolatey" CACHE PATH "")
> set(ZMK_CONFIG "C:/Users/as176/Desktop/Code/TRAE/KEYPAD/config" CACHE PATH "")
> ```

### Docker 构建
```powershell
.\build-local.ps1
```

## 已应用的关键修复
1. **Physical Layout 适配** — `numpad.overlay` 使用 `zmk,physical-layout`（main 分支新系统，旧 `zmk,matrix-transform` 已失效）
2. **模块入口分离** — `extra-module/zephyr/module.yml`（`board_root: ..`）避免项目根目录 `zephyr/` 自递归
3. **编码器引脚调整** — 从 D0/D1 移到 D7/D8，释放串口
4. **att.c C99 修复** — `zephyr/subsys/bluetooth/host/att.c` 第 731 行 `default:` 后加空语句（GCC 10 兼容）

## 构建产物
- 路径：`build/zephyr/zmk.uf2`
- 大小：645120 字节
- 内存：FLASH 39.76% / RAM 21.11%
- 刷入：拖入 nice!nano v2 USB 存储器

## 提交记录
- `56ee32e` feat: 初始化数字小键盘 ZMK 工程
- `6f6b747` fix: 补 config/west.yml 并改用 nice_nano//zmk variant
- `f8c2a01` feat: 适配 ZMK main 分支并支持本地工具链构建

## 下一步可做
- 烧录测试
- 启用串口调试（D0/D1 已释放）
- 深睡眠 `CONFIG_ZMK_SLEEP=y`（`config/numpad.conf` 已注释）
- 扩展未使用引脚功能
