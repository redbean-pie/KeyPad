# 项目备注

## 项目概况
- **类型**：ZMK 数字小键盘固件
- **板子**：nice!nano v2（nRF52840，Pro Micro 兼容接口）
- **Shield**：numpad（6×4 矩阵含编码器按下虚拟行 + EC11 旋钮 + AI32C 触摸滑块）
- **当前状态**：标准 numpad 布局 + AI32C 触摸集成，编译通过 ✅

## 引脚分配

> 分配原则：每类信号使用排针上物理连续的引脚。左列 D2-D9 连续，右列 D10-D19 连续。

| 功能 | Pro Micro | 物理位置 | nRF52840 |
|------|-----------|----------|----------|
| 矩阵行 R0 | D10 | 右13 | P0.9 |
| 矩阵行 R1 | D16 | 右12 | P0.10 |
| 矩阵行 R2 | D14 | 右11 | P1.11 |
| 矩阵行 R3 | D15 | 右10 | P1.13 |
| 矩阵行 R4 | D18 | 右9 | P1.15 | 含编码器按压（R4C0）|
| 矩阵列 C0 | D2 | 左6 | P0.17 |
| 矩阵列 C1 | D3 | 左7 | P0.20 |
| 矩阵列 C2 | D4 | 左8 | P0.22 |
| 矩阵列 C3 | D5 | 左9 | P0.24 |
| AI32C OUT1 | D6 | 左10 | P1.0 | 触摸数据通道1 |
| AI32C OUT2 | D7 | 左11 | P0.11 | 触摸数据通道2 |
| EC11 A 相 | D8 | 左12 | P1.4 |
| EC11 B 相 | D9 | 左13 | P1.6 |
| WS2812 DATA | D20 | 右7 | P0.29 | RGB 灯珠数据（SPIM3 MOSI）|
| WS2812 电量 | D19 | 右8 | P0.2 | 电量灯数据（SPIM2 MOSI）|
| UART RX（保留） | D0 | 左3 | P0.8 |
| UART TX（保留） | D1 | 左2 | P0.6 |

**编码器按压**：复用矩阵 R4C0（0 键 2U 宽的空位，行 D18 + 列 D2），不再需要独立虚拟行。

**WS2812**：
- 主链：每键 1 颗共 19 颗，`chain-length=19`，DATA=D20（SPIM3），rgb_underglow 管理
- 电量链：2 颗独立，`chain-length=2`，DATA=D19（SPIM2），`extra-module/battery_led` 模块管理，进 fn 层显示 2 秒
- 供电从 RAW 取（勿用 3.3V），每 5-10 颗加 100nF 去耦

**未使用引脚**：D21（右6）空闲可扩展。D0/D1 保留给串口调试，不占用。

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
3. **编码器引脚调整** — 从 D0/D1 移到 D8/D9，释放串口
4. **att.c C99 修复** — `zephyr/subsys/bluetooth/host/att.c` 第 731 行 `default:` 后加空语句（GCC 10 兼容）
5. **AI32C kscan driver** — 自定义 2 GPIO 解码驱动（`extra-module/drivers/kscan/kscan_gpio_ai32c.c`）
6. **引脚重排** — 按排针物理位置连续分配，所有信号组物理连续
7. **AI32C 中断驱动 + PM** — 空闲时 OUT GPIO 中断等待触摸（无轮询空转），触摸期间轮询监视释放；声明 PM + `wakeup-source` 支持深睡眠触摸唤醒

## 低功耗机制
- **Idle（30s 无操作）**：RGB 自动关（需 `CONFIG_ZMK_RGB_UNDERGLOW_AUTO_OFF_IDLE=y`），AI32C 中断驱动无空转
- **Deep Sleep（可选）**：开 `CONFIG_ZMK_SLEEP=y` + `CONFIG_ZMK_IDLE_SLEEP_TIMEOUT`，USB 拔出且空闲超时后 `sys_poweroff`；矩阵按键和触摸滑块（wakeup-source）均可唤醒
- 蓝牙：Idle 保持连接，Deep Sleep 断开

## 构建产物
- 路径：`build/zephyr/zmk.uf2`
- 大小：403456 字节（含 AI32C driver）
- 内存：FLASH 24.87% / RAM 18.50%
- 刷入：拖入 nice!nano v2 USB 存储器

## 提交记录
- `f1ebb22` feat: 集成 AI32C 触摸滑块 kscan driver
- `81c033e` feat: 标准 numpad 布局 — 删 FN 键、0 改 2U 宽、编码器按下切层，文档迁移至 docs/
- `a830daf` feat: 重新设计布局并实现编码器双模式切换
- `4c48d9a` docs: 添加项目备注文档
- `f8c2a01` feat: 适配 ZMK main 分支并支持本地工具链构建
- `6f6b747` fix: 补 config/west.yml 并改用 nice_nano//zmk variant
- `56ee32e` feat: 初始化数字小键盘 ZMK 工程

## 下一步可做
- 烧录测试：杜邦线模拟 AI32C 输出（D6/D7 碰 GND）验证解码
- PCB 打板后真实触摸测试
- 启用 deep sleep：`CONFIG_ZMK_SLEEP=y`（AI32C 驱动已支持触摸唤醒）
- 启用 idle 自动关 RGB：`CONFIG_ZMK_RGB_UNDERGLOW_AUTO_OFF_IDLE=y`
- 启用串口调试（D0/D1 已保留）
