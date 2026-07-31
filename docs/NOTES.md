# 项目备注

## 项目概况
- **类型**：ZMK 数字小键盘固件
- **板子**：nice!nano v2（nRF52840，Pro Micro 兼容接口）
- **Shield**：numpad（6×4 矩阵含编码器按下虚拟行 + EC11 旋钮 + AI32C 触摸滑块）
- **当前状态**：标准 numpad 布局（删 FN、0 改 2U 宽、编码器按下切层），编译通过 ✅

## 引脚分配

| 功能 | Pro Micro | nRF52840 |
|------|-----------|----------|
| 矩阵行 R0 | D20 / A2 | P0.29 |
| 矩阵行 R1 | D16 | P0.10 |
| 矩阵行 R2 | D14 | P1.11 |
| 矩阵行 R3 | D15 | P1.13 |
| 矩阵行 R4 | D18 / A0 | P1.15 |
| 矩阵行 R5（编码器按下）| D10 / A10 | P0.9 |
| 矩阵列 C0 | D9 / A9 | P1.6 |
| 矩阵列 C1 | D6 / A7 | P1.0 |
| 矩阵列 C2 | D5 | P0.24 |
| 矩阵列 C3 | D4 / A6 | P0.22 |
| EC11 A 相 | D7 | P0.11 |
| EC11 B 相 | D8 / A8 | P1.4 |
| AI32C OUT1 | D2 | P0.17 | 触摸数据通道1 |
| AI32C OUT2 | D3 | P0.20 | 触摸数据通道2 |
| UART RX（空闲） | D0 | P0.8 |

**未使用引脚**：D19/A1（预留拨动开关）、D21/A3 可作扩展。D0 空闲（UART RX 备用）。D1 已释放（原误分配给 AI32C T3）。D2/D3 已分配给 AI32C OUT1/OUT2。

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
- 大小：648192 字节
- 内存：FLASH 39.94% / RAM 21.34%
- 刷入：拖入 nice!nano v2 USB 存储器

## 提交记录
- `56ee32e` feat: 初始化数字小键盘 ZMK 工程
- `6f6b747` fix: 补 config/west.yml 并改用 nice_nano//zmk variant
- `f8c2a01` feat: 适配 ZMK main 分支并支持本地工具链构建
- `a830daf` feat: 重新设计布局并实现编码器双模式切换

## 下一步可做
- 编写 AI32C 自定义 ZMK kscan driver（2 GPIO 解码为 3 键）
- 编译验证 AI32C driver 集成后固件正常
- 烧录测试触摸滑块三通道
- 提交当前标准布局改动
- 启用串口调试（D0 已释放）
- 深睡眠 `CONFIG_ZMK_SLEEP=y`（`config/numpad.conf` 已注释）
