# Findings & Decisions

## Requirements
- AI32C 三通道电容触摸滑块集成到 numpad 固件
- 替代已移除的 OLED
- 触摸通道映射到多媒体按键（静音/播放暂停/切层等）

## AI32C 芯片规格（已确认 ✅）

### 引脚定义（SOP-8）
| 脚位 | 名称 | 功能 | 连接 |
|------|------|------|------|
| 1 | C1 | 内部基准电容 | 4.7nF NPO（1nF-10nF 可调）→ GND |
| 2 | KEY1 | 触摸通道 1 | 触摸感应盘 |
| 3 | KEY2 | 触摸通道 2 | 触摸感应盘 |
| 4 | KEY3 | 触摸通道 3 | 触摸感应盘 |
| 5 | OUT2 | 数据通道 2 输出 | D7 / P0.11（1K 上拉至 VDD） |
| 6 | OUT1 | 数据通道 1 输出 | D6 / P1.0（1K 上拉至 VDD） |
| 7 | VDD | 正电源 | 3.3V（串 20Ω，不可省） |
| 8 | GND | 电源地 | GND |

### 输出编码表（低有效）
| 触摸按键 | OUT2 (pin5) | OUT1 (pin6) |
|----------|-------------|-------------|
| KEY1 | 0 | 0 |
| KEY2 | 0 | 1 |
| KEY3 | 1 | 0 |
| 无触摸 | 1 | 1 |

多键同时触摸：优先级 KEY1 > KEY2 > KEY3

### 电气参数
- 供电：2.5-5.5V（nice!nano 3.3V ✅）
- 工作电流：394µA @ 3V
- 睡眠电流：7µA @ 3V
- 上电初始化：300ms
- 感应电容范围：0.2-100pF
- ESD：≥8000V
- 内置按键消抖（AI32C 独有）

### 外围电路
- C1：4.7nF NPO 材质，尽量贴近 IC
- VDD-GND：100nF 退耦电容
- VDD 串联：20Ω 电阻（不可省）
- OUT1/OUT2：各 1K 上拉电阻
- KEY1/2/3：可选对地灵敏度电容 1-100pF（不接时灵敏度最高，建议保留 PCB 焊盘）

### AI32C vs AI32
| 参数 | AI32 | AI32C |
|------|------|-------|
| 睡眠电流 | 45µA | 7µA |
| 工作电流 @3V | 800µA | 394µA |
| 初始化时间 | 400ms | 300ms |
| 内置消抖 | 无 | 有 |
| C1 范围 | 固定 4.7nF | 1nF-10nF |

AI32C 是 AI32 的低功耗增强版，引脚兼容。

## 引脚分配决策（重排后）
| 信号 | Pro Micro | nRF52840 | 说明 |
|------|-----------|----------|------|
| OUT1 | D6 | P1.0 | AI32C 数据通道 1 |
| OUT2 | D7 | P0.11 | AI32C 数据通道 2 |
| 列 C0-C3 | D2/D3/D4/D5 | P0.17/P0.20/P0.22/P0.24 | 矩阵列（左列连续）|
| 行 R0-R4 | D10/D16/D14/D15/D18 | P0.9/P0.10/P1.11/P1.13/P1.15 | 矩阵行（右列连续），R4C0=编码器按压 |
| EC11 A/B | D8/D9 | P1.4/P1.6 | 编码器 |
| WS2812 主链 | D20 | P0.29 | SPIM3 MOSI，19 颗 RGB |
| WS2812 电量链 | D19 | P0.2 | SPIM2 MOSI，2 颗电量/充电灯 |
| 蓝牙 LED | D21 | P0.31 | GPIO 输出，蓝灯 |
| 预留 | D0/D1 | P0.8/P0.6 | 串口（UART）|

原计划 D1→T3 是错误的——AI32C 没有独立 T3 输出，3 个通道通过 2 根线编码。

## Technical Decisions
| Decision | Rationale |
|----------|-----------|
| AI32C 替代 OLED 而非共存 | OLED 已完全移除，释放 I2C 引脚 |
| D6→OUT1, D7→OUT2（仅 2 GPIO）| AI32C 为 2 线编码输出，非 3 路独立 |
| 需自定义 ZMK kscan driver | ZMK 无内置二进制解码 kscan |
| AI32C 中断+轮询混合驱动 | 空闲无轮询空转（省电）+ PM 深睡眠触摸唤醒 |
| WS2812 主链/电量链独立 SPI | rgb_underglow 50ms 覆盖整链，共享会冲突 |
| 蓝牙 LED 直驱 D21 | 独立单色灯，不占 WS2812 电量灯 |
| 低功耗 Idle+Deep Sleep 已启用 | AI32C 自睡眠 7µA + ZMK 自动待机/断电 |
| 电源开关 DPDT 正负极双断 | 彻底断电；USB 供电路径独立 |
| nice!nano 3.3V 直接供电 AI32C | 在 2.5-5.5V 范围内 |
| **滑条 sensor 方案废弃（2026-08-06）** | 死机 + 无反应：holding 20Hz 触发堵塞 behavior queue；依赖 ZMK `behavior_sensor_rotate_common.c` 第 29 行 legacy compat 路径（标记 REMOVE ME）未来失效；PM RESUME 死锁边界 |
| **回退 kscan 触摸键方案** | 3 个独立按键，每层独立功能；ZMK 标准 kscan API 稳定，无 legacy 依赖 |

## 滑条方案 vs kscan 触摸键方案对比

| 维度 | 滑条 sensor（已废弃） | kscan 触摸键（当前） |
|------|-------|-------|
| ZMK API | sensor_driver_api + trigger_set | kscan_driver_api |
| 数据流 | OUT1/OUT2 → zone 解码 → delta → sensor_value | OUT1/OUT2 → key 解码 → kscan_callback |
| 触发模式 | 持续触发（holding 时重复） | 按下/松开事件（一次性） |
| keymap 绑定 | sensor-bindings（inc_dec_kp） | 普通 keymap 矩阵位置 |
| 切层能力 | 无法切层（sensor 不参与 layer） | 可切层（按键可绑 &to N） |
| 行为队列压力 | 高（holding 20Hz × 2 queue_add） | 低（按下 1 次 1 个事件） |
| ZMK 兼容性 | 依赖 legacy compat 路径（待移除） | 标准 kscan，长期稳定 |
| PM 唤醒 | 支持 | 支持 |

## Issues Encountered
| Issue | Resolution |
|-------|------------|
| 本地 AI32C PDF 为扫描图片无法 OCR | 从 xmxwdz.cn 下载文本版 PDF，pypdf 成功提取全部内容 |
| szlcsc 页面需 JS 渲染，Firecrawl/WebFetch 均失败 | firecrawl-search 找到 xonstorage 和 xmxwdz 替代源 |
| pdftoppm 未安装 | 跳过渲染，直接用 pypdf 处理文本版 PDF |

## Resources
- AI32C 数据手册（文本版）：`http://www.xmxwdz.cn/home/d/5/mr3ytu/resource/2023/11/23/655f38887b2ea.pdf`
- AI32 数据手册（对比参考）：`https://xonstorage.z8.web.core.windows.net/pdf/samwing_ai32_lcs01_linknew.pdf`
- 项目仓库：`C:\Users\as176\Desktop\Code\TRAE\KEYPAD`
- ZMK 官方文档：https://zmk.dev/docs
- ZMK kscan driver 开发参考：https://zmk.dev/docs/development/new-shield#kscan-driver

## Visual/Browser Findings
- AI32C PDF 共 7 页，全部文本可提取
- 引脚图在 PDF 第 3 页（page 2 末尾有封装图）
- 典型应用电路图在 PDF 第 3 页：VDD 串 20Ω → 100nF 退耦 → C1(4.7nF) → OUT1/OUT2 各 1K 上拉

---
*Update this file after every 2 view/browser/search operations*
*This prevents visual information from being lost*

## 2026-08-06 补充发现：AI32C 输出类型 + LDO 损坏根因

### AI32C OUT 是 push-pull 输出（非开漏）
- 数据手册典型应用电路图虽画了 1K 上拉，但实测 AI32C OUT 空闲时主动输出高电平（接近 VDD）
- **不需要 PULL_UP**：驱动改为纯输入（gpio_pin_configure 忽略 dt_flags）
- 飞线 5V 供电时，PULL_UP 拉到 3.3V 会与 5V 输出冲突，必须去掉 PULL_UP

### nice!nano 3.3V VCC 关闭（触摸无反应根因，2026-08-08 纠正）
- **现象**：AI32C VDD=0V，OUT 输出默认低(00)被误判 KEY1 持续触摸
- **排查**：init 时 PULL_UP 拉高 raw=1（证明引脚没短路），但运行后 poll 读到 00；高阻模式仍 00 -> AI32C 自己输出
- **根因（纠正）**：**不是 LDO 损坏**。万用表实测 LDO（ME6211C33M5G-N，用户已换全新）三脚：IN=4.86V（正常），**CE=0V（异常）**，OUT=0V。CE=0V 关闭 LDO，OUT 才为 0V。CE 由 P0.13 控制（原理图标注 P0.13-POWER-EN，下拉 R2=100K），开发板手册明确"P0.13 设低关闭 3.3V VCC"
- **验证**：VCC 对 GND 高阻（无短路），AI32C pin7/8 高阻（无击穿）→ 排除短路，确认是 CE 控制问题
- **临时修复**：飞线 nice!nano RAW（4.1V 电池/5V USB）-> AI32C VDD，通电后空闲变 11，触摸恢复
- **代码层尝试（失败）**：见下方 EXT_POWER 段

### GPIOTE 中断不触发（已用 poll 绕过）
- EDGE_BOTH 中断配置成功（err=0），但短接/触摸不触发 IRQ
- 原因可能：ZMK composite 不调 enable_callback（已改 init 启动）；或 GPIOTE 通道问题
- **解决方案**：改用 poll 模式（10ms），彻底绕开中断，已验证可靠

### 编码表修正（findings.md 第 23-28 行列顺序）
数据手册原表（OUT2 在前，OUT1 在后）：
| 触摸 | OUT2 | OUT1 |
|------|------|------|
| KEY1 | 0 | 0 |
| KEY2 | 0 | 1 |
| KEY3 | 1 | 0 |
| 无   | 1 | 1 |

驱动读取顺序：o1=OUT1(pin6/D6), o2=OUT2(pin5/D7)，编码(o1,o2)：
- 00=KEY1  10=KEY2  01=KEY3  11=空闲
（实测验证：摸 KEY2 得 o1=1 o2=0，摸 KEY3 得 o1=0 o2=1，符合）

## 2026-08-06 补充发现：三层 keymap 编译的 4 个阻塞根因

### 1. `spi1_sleep lacks #sensor-binding-cells` = keymap 的 phandle-array cell 错位
- 触发机制：`sensor-bindings` 是 phandle-array，specifier 由属性名去掉末尾 's' 得 "sensor-binding"，每个 phandle 目标需有 `#sensor-binding-cells`（edtlib `_phandle_val_list`，`_err(f"{node!r} lacks {full_n_cells_name}")`）
- 根因：`RGB_BRI`/`RGB_BRD` 宏各展开成 2 值（`RGB_BRI_CMD 0` = `7 0`），`&inc_dec_kp RGB_BRI RGB_BRD` 变成 phandle+4 cells 而 inc_dec_kp 只吃 2，错位把 `8` 当 phandle 解析到 spi1_sleep
- 关键认知：**ZMK rgb.h 里 RGB_* 宏是给 rgb_ug 用的双值宏（cmd+arg），不能当 inc_dec_kp 的 sensor 参数**
- 修复：自定义非 var 版 `zmk,behavior-sensor-rotate`（`bindings=<&rgb_ug RGB_BRI>,<&rgb_ug RGB_BRD>`），rgb_layer 用 `<&inc_dec_rgb>`。非 var 版 `#sensor-binding-cells=0`，参数内联进 bindings

### 2. Kconfig 递归 = ZMK_EXTRA_MODULES 污染 + 根 module.yml 无 kconfig
- `zephyr/module.yml`（用户有意创建、被 git 跟踪，只有 `board_root: .`）用于把 workspace 根注册为 board root（发现 numpad shield）
- 但它没有 cmake/kconfig：当 `ZMK_EXTRA_MODULES` 被污染成 workspace 根时（构建不带 `-C zmk-cache.cmake`），Zephyr 把根当模块、Kconfig 默认落到 `zephyr/Kconfig` 自身 → 递归
- 修复：固定用带 `-C zmk-cache.cmake` 的命令（`ZMK_EXTRA_MODULES=extra-module`），根不再被扫描。文件保留

### 3. `extra-module__drivers__kscan` target 不存在 = 旧的 zephyr_library_amend()
- kscan CMakeLists 用 `zephyr_library_amend()`（无当前库上下文报错），改 `zephyr_library()` + include 目录

### 4. 正确构建命令必须带 `-C zmk-cache.cmake`
- 全量：`west build -s zmk/app -b nice_nano//zmk -d build -- -C zmk-cache.cmake -DSHIELD=numpad`
- 增量 `west build -d build` 只在缓存未污染时可用

## 2026-08-08 补充发现：P0.13 / NFCT / EXT_POWER 尝试（失败）

### P0.13 是 nRF52840 NFC 引脚
- nRF52840 的 P0.09/P0.10 默认是 NFC 天线引脚（NFCT），不是普通 GPIO
- **注意**：实际控制 LDO CE 的是 P0.13（原理图 P0.13-POWER-EN），P0.13 本身不是 NFCT 引脚（NFCT 是 P0.09/P0.10）。但 nice!nano 设计中 P0.13 控制 EXT-VCC
- 开发板手册原文："当 P0.13 设置为低时，将关闭 3.3V、VCC 引脚的电源。这对于减少空闲时使用功率的组件（如 RGB、LED）非常有用"

### EXT_POWER 尝试（失败）
1. `numpad.overlay` 加 `EXT_POWER` 节点：`compatible = "zmk,ext-power-generic"; control-gpios = <&gpio0 13 GPIO_ACTIVE_HIGH>`
2. `numpad.conf` 加 `CONFIG_ZMK_EXT_POWER=y`（确认生成 `CONFIG_ZMK_EXT_POWER=1`、`CONFIG_DT_HAS_ZMK_EXT_POWER_GENERIC_ENABLED=1`）
3. ZMK ext-power 驱动 `zmk/app/src/ext_power_generic.c`：init 时 `ext_power_enable()` 调 `gpio_pin_set_dt(gpio, 1)` 拉高，优先级 81（POST_KERNEL）
4. 启动日志**无 ext-power 相关输出**，CE 实测仍 0V
5. 驱动 init 里手动 `gpio_pin_configure` + `gpio_pin_set` 拉高 P0.13，烧录后仍无效

### 失败原因（推测，未最终定位）
- ext-power 驱动 init 未打印日志，可能未真正执行到 enable
- 或 P0.13 走线/硬件问题，固件拉高但物理不到 CE 脚
- 代码层无法进一步定位，需硬件排查 P0.13 到 CE 的走线

### 最终决策
- `git restore` 回退 EXT_POWER 改动，工作区干净 = HEAD `3ec4de0`
- 代码层不再尝试修复供电，交硬件层处理（排查 P0.13 走线 / 外接 AMS1117-3.3 绕过板载 LDO）
- 固件可用状态：飞线 RAW 供电时三键触摸触发音量增加（已验证）
