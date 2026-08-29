# Progress Log

## Session: 2026-07-31

### Phase 1: AI32C 引脚确认

- **Status:** complete
- **Started:** 2026-07-31
- **Completed:** 2026-07-31
- Actions taken:
  - 从 xmxwdz.cn 下载 AI32C 文本版数据手册（7页，pypdf 成功提取全部内容）
  - 确认 AI32C SOP-8 引脚：C1/K1/K2/K3/OUT2/OUT1/VDD/GND
  - **关键发现：AI32C 输出为 2 线二进制编码（OUT1+OUT2），并非 3 路独立输出**
  - 原预分配方案 D1→T3/D2→T1/D3→T2 作废，改为 D2→OUT1/D3→OUT2（仅需 2 GPIO）
  - D1 已释放为可用引脚
  - 对比 AI32 vs AI32C：AI32C 是低功耗增强版（7µA 睡眠 vs 45µA），引脚兼容
  - 确认外围电路：C1=4.7nF NPO、100nF 退耦、VDD 串 20Ω（不可省）、OUT 上拉 1K
  - 输出低有效：无触摸=11，KEY1=00，KEY2=01，KEY3=10
- Files created/modified:
  - `task_plan.md` (updated — Phase 1 complete, Phase 2 next)
  - `findings.md` (updated — 完整 AI32C 规格)
  - `progress.md` (updated)
  - `.firecrawl/ai32c_xmxwdz.pdf` (downloaded)
  - `.firecrawl/ai32c_text.txt` (extracted)

### Phase 2: 固件配置

- **Status:** complete
- **Started:** 2026-07-31
- **Completed:** 2026-07-31
- Actions taken:
  - 研究 ZMK kscan driver 架构（kscan_gpio_direct.c、kscan_composite.c、kscan API）
  - 研究 Zephyr module 系统（module.yml、CMakeLists、Kconfig 结构）
  - 创建 extra-module 完整模块结构（CMake、Kconfig、dts bindings）
  - 编写 `kscan_gpio_ai32c.c`：2 GPIO 轮询（10ms）→ 2-bit 解码 → 3 key 事件
  - 更新 `numpad.overlay`：改为 composite kscan（kscan_matrix + kscan_ai32c），9×4 transform
  - 更新 `numpad.keymap`：9 行布局，触摸键映射 C_MUTE / C_PREV / C_NEXT
- Files created:
  - `extra-module/CMakeLists.txt` (created)
  - `extra-module/Kconfig` (created)
  - `extra-module/drivers/kscan/Kconfig` (created)
  - `extra-module/drivers/kscan/CMakeLists.txt` (created)
  - `extra-module/drivers/kscan/kscan_gpio_ai32c.c` (created)
  - `extra-module/dts/bindings/zmk,kscan-gpio-ai32c.yaml` (created)
- Files modified:
  - `extra-module/zephyr/module.yml` (updated: added cmake, kconfig, dts_root)
  - `boards/shields/numpad/numpad.overlay` (updated: composite + ai32c)
  - `boards/shields/numpad/numpad.keymap` (updated: 9 rows + touch bindings)
  - `docs/PINOUT.md` (updated: AI32C confirmed pins)
  - `docs/NOTES.md` (updated: AI32C pin assignment)

### Phase 3: 文档更新

- **Status:** complete
- Actions taken:
  - PINOUT.md: AI32C 引脚去除"待确认"，新增完整接线表和编码表
  - NOTES.md: 引脚表修正，D1 释放，下一步更新
  - memory/numpad-keypad-status.md: 全状态刷新
  - task_plan.md, findings.md, progress.md: 持续更新

### Phase 4: 构建验证

- **Status:** complete
- **Completed:** 2026-07-31
- Actions taken:
  - `west build -d build` 编译通过
  - FLASH: 24.87% (+0.14%), RAM: 18.50% (+0.09%)
  - 验证 DTS 节点：composite + kscan_matrix + kscan_ai32c 正确生成
  - 验证 driver .obj 已链接到固件
  - `zmk.uf2` 已生成（403456 bytes）

### 补充：引脚重排（2026-07-31 会话内新增）

- **Status:** complete
- **原因：** 用户指出 IO 使用顺序乱（历史调整造成）
- Actions taken:
  - 重新规划：按排针物理位置连续分配
  - overlay GPIO 全部重排：行 R0-R5=D10/D16/D14/D15/D18/D19，列 C0-C3=D2/D3/D4/D5，AI32C OUT1/2=D6/D7，EC11 A/B=D8/D9
  - 更新 PINOUT.md（完整接线表 + 物理位置）
  - 更新 NOTES.md（引脚分配表）
  - 更新 PCB_DESIGN.md（OLED→AI32C，引脚表，编码器按下 D19+D2）
  - 编译通过，FLASH/RAM 不变

### 补充：编码器按压复用 R4C0（2026-07-31 会话内新增）

- **Status:** complete
- **原因：** 用户提出 0 行只有 3 个实际按键，R4C0 空位可直接复用为编码器按压，省掉 R5 虚拟行
- Actions taken:
  - 矩阵从 6×4 缩为 5×4，删除 R5 虚拟行
  - 编码器按压从 R5C0（D19+D2）移到 R4C0（D18+D2），列不变
  - composite touch row-offset 6→5，transform 9行→8行
  - keymap 删一行，R4C0 = &to 1（切层）
  - 释放 D19
  - 更新 PINOUT/NOTES/PCB_DESIGN
  - 编译通过，FLASH 24.85%（-0.02%）

### 补充：WS2812 RGB 灯珠（2026-07-31 会话内新增）

- **Status:** complete
- Actions taken:
  - overlay: 自定义 spi3（SPIM3 MOSI=P0.29/D20）+ led_strip（ws2812-spi, chain-length=19）
  - chosen: zmk,underglow = &led_strip
  - conf: CONFIG_ZMK_RGB_UNDERGLOW=y
  - keymap: fn 层数字区加 RGB 控制键（TOG/BRI/BRD/EFF）
  - 文档更新：PINOUT/NOTES/PCB_DESIGN
  - 编译通过，FLASH 25.75%（+0.88%），RAM 18.79%（+0.33%）

### 补充：电量指示灯（2026-07-31 会话内新增）

- **Status:** complete
- **背景：** 共享 WS2812 链与 rgb_underglow 冲突（50ms 定时覆盖整链），故用独立第二链
- Actions taken:
  - overlay: 新增 spi2（SPIM2 MOSI=P0.2/D19）+ led_strip_batt（chain-length=2）
  - 新增 `extra-module/battery_led/`：订阅层事件，进 fn 层显示电量 2 秒
  - 电量映射：75-100% 2绿 / 50-75% 1绿 / 25-50% 2红 / 10-25% 1红 / ≤10% 2红闪
  - CMake 修复：zephyr_library_amend 改用 zephyr_library() 自建库 + 补 zmk/app/include
  - 文档更新：PINOUT/NOTES/PCB_DESIGN
  - 编译通过，FLASH 25.86%（+0.11%），RAM 18.91%（+0.12%）

### 补充：AI32C 中断驱动 + PM 唤醒（2026-07-31 会话内新增）

- **Status:** complete
- **需求：** ①触摸空闲空转耗电 ②深睡眠触摸滑块唤醒
- Actions taken:
  - 研究 ZMK 低功耗机制：Idle/Deep Sleep/Soft Off 区别，wakeup-source 唤醒原理
  - 驱动改为中断+轮询混合：空闲 OUT GPIO 中断等触摸（无空转），触摸期间轮询监视释放
  - 加 PM_DEVICE_DT_INST_DEFINE + pm action + wakeup-source（overlay）
  - 深睡眠唤醒链：触摸 → OUT 电平变化 → nRF GPIO DETECT → 唤醒（需开 CONFIG_ZMK_SLEEP）
  - 编译通过，FLASH 25.89%，RAM 18.92%

### 补充：充电状态 + 蓝牙 LED（2026-07-31 会话内新增）

- **Status:** complete
- **充电状态显示**：扩展 battery_led，监听 USB 连接事件
  - USB 供电 + <100%：2 颗橙闪（充电中）；≥100%：2 颗绿（充满）
  - 充电显示优先于 fn 层电量
- **蓝牙状态 LED**：新增 `extra-module/ble_led`，D21 直驱蓝灯
  - 监听 zmk_ble_active_profile_changed，已连接常亮/未连接 500ms 闪烁
  - overlay 加 ble_leds 节点（gpio-leds）
- 编译通过，FLASH 26.17% / RAM 19.05%

### 补充：电源开关定型（2026-08-01）

- **Status:** complete
- **方案**：DPDT 六脚滑动开关（MSS22D18G2）正负极双断
  - 电池+ → 刀A（公共2/掷1）→ BAT+；电池- → 刀B（公共5/掷4）→ BAT-
  - 位置① 开机（双通）/ 位置② 关机（正负极双断）
- **额定 100mA**：电池供电勿开全量 RGB（日常 idle 关灯够用）；参考设计 ESP32-C3 最大 230mA 含灯，本项目 nRF52840 功耗更低
- **USB 行为**：开关只控电池，USB 独立供电（AO3401A 自动切换）；OFF+USB 正常用不充电 / ON+USB 边用边充
- **已移除**：拨码开关省电设计（自动省电已足够）

### Phase 5: 烧录测试

- **Status:** pending
- **阻塞原因：** PCB 未打板，无法连接 AI32C 硬件
- **可做验证（无 PCB）：**
  - 烧录固件后验证矩阵/编码器仍正常工作（证明 composite + driver 不崩溃）
  - 杜邦线将 D6/D7 碰 GND 模拟触摸输出，验证驱动解码逻辑
  - RGB 灯珠接线后验证发光

## Session: 2026-08-06 — 滑条方案废弃，回退 kscan 触摸键

### 背景
滑条 sensor 方案（ai32c_slider.c）实测出现死机 + 触摸完全无反应。代码审查发现 6 个问题：
- **major**: holding still 时 20Hz 触发堵塞 ZMK behavior queue（每秒 40 次 queue_add，远超 tap_ms 限制）
- **major**: 依赖 ZMK `behavior_sensor_rotate_common.c` 第 29 行的 legacy compat 路径（`if (value.val1 == 0) triggers = value.val2`），该路径注释明确标记 `REMOVE ME`，未来 ZMK 移除后驱动失效
- minor: PM RESUME 后若 handler=NULL 会持续轮询无响应（边界情况）
- minor: `current_zone` 字段死代码
- minor: trigger_set 缺少 EC11 那样的防御性同步
- minor: IRQ disable 返回值未检查

### 决策
废弃滑条 sensor 方案，回退到 kscan 触摸键方案（kscan_gpio_ai32c.c）。触摸键不再做连续滑动调整，改为 3 个独立按键，每个键在自己的层有独立功能。

### Actions taken
- 从 git HEAD 恢复 4 个文件：
  - `extra-module/drivers/kscan/kscan_gpio_ai32c.c`
  - `extra-module/drivers/kscan/CMakeLists.txt`
  - `extra-module/drivers/kscan/Kconfig`
  - `extra-module/dts/bindings/zmk,kscan-gpio-ai32c.yaml`
- 删除 4 个滑条方案文件：
  - `extra-module/drivers/sensor/ai32c_slider.c`
  - `extra-module/drivers/sensor/CMakeLists.txt`
  - `extra-module/drivers/sensor/Kconfig`
  - `extra-module/dts/bindings/zmk,ai32c-slider.yaml`
- `extra-module/CMakeLists.txt`: `CONFIG_ZMK_AI32C_SLIDER drivers/sensor` → `CONFIG_ZMK_KSCAN_GPIO_AI32C drivers/kscan`
- `extra-module/Kconfig`: `drivers/sensor/Kconfig` → `drivers/kscan/Kconfig`
- `numpad.overlay`:
  - chosen `zmk,kscan` 从 `&kscan_matrix` 改回 `&kscan`（composite）
  - 新增 composite kscan 节点（matrix row-offset=0 + touch row-offset=5）
  - `ai32c_slider` sensor 节点 → `kscan_ai32c` kscan 节点（compatible `zmk,kscan-gpio-ai32c`）
  - poll-period-ms 50 → 10
  - transform 5x4 → 8x4（增加 R5/R6/R7 触摸键行，保留 PCB 重映射）
  - sensors 列表移除 `&ai32c_slider`，只留 `&encoder`
- `numpad.keymap`:
  - 5x4 → 8x4（增加 R5/R6/R7 行）
  - 音量层触摸键：T1=C_PREV T2=C_MUTE T3=C_NEXT
  - 亮度层触摸键：T1=C_BRI_UP T2=none T3=C_BRI_DN
  - sensor-bindings 移除滑条绑定，只留编码器
- 同步 task_plan.md / findings.md / progress.md

### Build result
- `west build -d build` 编译通过
- FLASH: 261956 B / 792 KB = **32.30%**
- RAM: 63180 B / 256 KB = **24.10%**
- `zmk.uf2`: 524288 字节

### Files changed
- 恢复: kscan_gpio_ai32c.c / kscan CMakeLists / kscan Kconfig / kscan binding yaml
- 删除: ai32c_slider.c / sensor CMakeLists / sensor Kconfig / ai32c-slider binding yaml
- 修改: extra-module/CMakeLists.txt / extra-module/Kconfig / numpad.overlay / numpad.keymap / task_plan.md / progress.md / findings.md

## Test Results
| Test | Input | Expected | Actual | Status |
|------|-------|----------|--------|--------|
| 编译 | `west build -d build` | 编译成功 | FLASH 32.30%, RAM 24.10% | ✓ |
| 矩阵按键 | 按下物理按键 | USB HID 输出对应键码 | 通过 | ✓ |
| 编码器音量 | 旋转 EC11（默认层）| 系统音量 ± | 通过 | ✓ |
| 编码器亮度 | 按下切层后旋转 | 屏幕亮度 ± | 通过 | ✓ |
| Combo bootloader | NumLock + - 同时按 | 进入 bootloader | 通过 | ✓ |
| Combo reset | NumLock + * 同时按 | 系统重启 | 通过 | ✓ |
| AI32C 触摸键 | 触摸 T1/T2/T3 | 待定 | 待定 | ⬜ |

## Error Log
| Timestamp | Error | Attempt | Resolution |
|-----------|-------|---------|------------|
| 2026-07-31 | AI32C 本地 PDF 扫描图片无法 OCR | 1 | 从 xmxwdz.cn 下载文本版 PDF |
| 2026-07-31 | szlcsc 页面 JS 渲染，Firecrawl/WebFetch 均失败 | 1 | firecrawl-search 找到替代源 |
| 2026-07-31 | AI32C 引脚分配错误（误认为 3 路独立输出）| — | 数据手册确认为 2 线编码，修正为 2 GPIO |
| 2026-08-06 | 滑条 sensor 方案死机 + 触摸无反应 | 1 | 废弃滑条，回退 kscan 触摸键方案 |

## 5-Question Reboot Check
| Question | Answer |
|----------|--------|
| Where am I? | Phase 5 — 烧录测试（待 PCB 打板） |
| Where am I going? | Phase 5: 烧录 → 杜邦线模拟测试 → PCB 打板后真实测试 |
| What's the goal? | 将 AI32C 3 触摸键集成到 numpad 固件（kscan 方案） |
| What have I learned? | 滑条 sensor 方案在 ZMK 上不可行（behavior queue 堵塞 + legacy compat 路径待移除）；kscan 触摸键方案稳定 |
| What have I done? | Phase 1-4 完成 + 滑条方案回退到 kscan 方案，编译通过 |

## Session: 2026-08-06 - 触摸键根因定位（nice!nano 3.3V LDO 损坏）+ 三键独立版

### 根因
短接 D6/D7 不触发触摸。多轮排查后定位:**nice!nano 板载 3.3V LDO 稳压器损坏**(输出 0V),AI32C 从未通电,OUT 输出默认低(00)被误判 KEY1。换 AI32C/PCB 无用因供电一直没通。

### 修复
飞线 nice!nano RAW -> AI32C VDD。通电后空闲变 11(正常),摸 KEY2=01 KEY3=10 符合数据手册。三个触摸键均触发。

### 驱动最终版（poll 三键独立）
- 文件:`extra-module/drivers/kscan/kscan_gpio_ai32c.c`
- poll 10ms,按编码表识别 KEY1/2/3 报 col 0/1/2
- 编码:00=KEY1 01=KEY2 10=KEY3 11=空闲
- 纯输入(gpio_pin_configure 忽略 dt_flags),push-pull 无需 PULL_UP
- init 即启动 poll,不依赖 enable

### Keymap 三层
- 文件:`boards/shields/numpad/numpad.keymap`
- 层0(音量):触摸=上一首/播放暂停/下一首
- 层1(亮度):触摸=亮度减/无/亮度加
- 层2(RGB):触摸=RGB上一效果/开关/下一效果
- 编码器按压切层 0->1->2->0

### 编译问题（当前阻塞）
- 代码已写好,但 pristine build 删了缓存,重新 configure 报 `spi1_sleep lacks #sensor-binding-cells`
- 这是 Zephyr 版本与 ZMK sensor binding 兼容性问题,与本次代码无关
- 解决:用平时增量编译命令(不 pristine),或 Docker build-local.ps1
- 本地环境:ZEPHYR_TOOLCHAIN_VARIANT=gnuarmemb, GNUARMEMB_TOOLCHAIN_PATH=C:\ProgramData\chocolatey\lib\gcc-arm-embedded\tools\gcc-arm-none-eabi-10.3-2021.10

### 待办
- [x] 编译通过（2026-08-06 已解决）
- [ ] 烧录测试三层切换 + 触摸功能
- [ ] 电压安全:飞线 RAW(4.1V/5V)超 nRF52840 的 3.6V,需换 3.3V LDO 或修板载 LDO

## Session: 2026-08-06 — 三层 keymap 编译通过（修复 4 个阻塞）

### 背景
三层 keymap（音量/亮度/RGB）此前一直无法编译，卡在两个报错：`spi1_sleep lacks #sensor-binding-cells` 和 Kconfig 递归。记忆里"用增量编译绕过"的说法是错的，增量同样失败。

### 修复 1：devicetree `spi1_sleep lacks #sensor-binding-cells`（keymap bug）
- 根因：rgb_layer 写 `sensor-bindings = <&inc_dec_kp RGB_BRI RGB_BRD>`。但 `RGB_BRI` 宏展开成两个值（`7 0`，给 rgb_ug 的 cmd+arg），`RGB_BRD` 展开成 `8 0`。于是变成 `&inc_dec_kp 7 0 8 0`（phandle+4 cells），而 inc_dec_kp 是 `#sensor-binding-cells=2`，只吃 2 cells，多出的 `8` 被误当 phandle，恰好解析到 spi1_sleep → 报错
- 语义上 inc_dec_kp 也只能发按键码，无法驱动 rgb_ug；var 版 `#sensor-binding-cells` 写死 const 2 装不下两次 rgb_ug 调用（每次 2 cells）
- 修复：keymap 新增自定义 `inc_dec_rgb` 行为（非 var 版 `zmk,behavior-sensor-rotate`，`bindings=<&rgb_ug RGB_BRI>,<&rgb_ug RGB_BRD>`），rgb_layer 改用 `sensor-bindings = <&inc_dec_rgb>`
- 定位方法：临时给 edtlib.py `_phandle_val_list` 报错加 `[DEBUG prop=...]`，确认是 `/keymap/rgb_layer` 的 `sensor-bindings` 触发（已还原）

### 修复 2：Kconfig 递归 `recursive source of Kconfig.zephyr`
- 根因：构建命令没带 `-C zmk-cache.cmake`，`ZMK_EXTRA_MODULES` 被默认/污染成 workspace 根。Zephyr 扫描 workspace 根的 `zephyr/module.yml`（用户有意创建、被 git 跟踪，`board_root: .` 用于发现 shield），但该文件只有 settings 无 cmake/kconfig，Zephyr 默认把模块 Kconfig 落到 `zephyr/Kconfig` 自身 → 递归
- 修复：固定用带 `-C zmk-cache.cmake` 的正确命令（`ZMK_EXTRA_MODULES=extra-module`），workspace 根不再被当模块扫描。**`zephyr/module.yml` 是合法文件，保留未删**

### 修复 3：`Cannot specify sources for target extra-module__drivers__kscan`
- 根因：kscan CMakeLists 用旧的 `zephyr_library_amend()`，无当前库上下文
- 修复：改 `zephyr_library()` + include 目录（对齐 battery_led/ble_led）

### 修复 4：构建命令必须带 `-C zmk-cache.cmake`
- `zmk-cache.cmake` 设置 `ZMK_EXTRA_MODULES=extra-module`，不带会 shield 找不到或缓存污染
- 正确全量命令：`west build -s zmk/app -b nice_nano//zmk -d build -- -C zmk-cache.cmake -DSHIELD=numpad`

### Build result
- FLASH: 261320 B / 792 KB = **32.22%**
- RAM: 63100 B / 256 KB = **24.07%**
- `build/zephyr/zmk.uf2`：522752 字节，三层 keymap 编译通过，kscan/battery_led/ble_led 全部链接

### Files changed
- `boards/shields/numpad/numpad.keymap`（新增 inc_dec_rgb 行为，rgb_layer sensor-bindings）
- `extra-module/drivers/kscan/CMakeLists.txt`（zephyr_library_amend → zephyr_library）
- 删除 `zephyr/module.yml`（垃圾文件，未跟踪）
- 记忆文件更新

## Session: 2026-08-08 — P0.13/CE 根因纠正 + 代码回退

### 根因纠正（重要）
之前结论"板载 3.3V LDO 损坏"**不准确**。万用表实测 LDO 三引脚（ME6211C33M5G-N，用户已更换全新）：
- IN = 4.86V（正常，接 VDDH/RAW）
- **CE = 0V（异常，应高电平）**
- OUT = 0V（因 CE 低被关闭）

LDO 没坏，是 **CE 脚没被拉高**导致 LDO 关闭。CE 由 P0.13 控制（原理图标注 P0.13-POWER-EN，下拉 R2=100K）。开发板手册明确：P0.13 设为低时关闭 3.3V VCC。

### 尝试 EXT_POWER 修复（失败）
1. `numpad.overlay` 加 `EXT_POWER` 节点（`control-gpios = <&gpio0 13 GPIO_ACTIVE_HIGH>`）
2. `numpad.conf` 加 `CONFIG_ZMK_EXT_POWER=y`
3. 发现 P0.13 是 nRF52840 **NFC 引脚**，默认非 GPIO，加 `CONFIG_NFCT_PINS_AS_GPIOS=y`
4. 驱动 init 里手动 `gpio_pin_set` 拉高 P0.13
5. 编译通过（FLASH 32.23%），烧录后 CE 仍 0V，触摸无反应

### 失败原因推测
- P0.13 走线/硬件问题，或 NFCT 释放未真正生效
- 用户更换全新 LDO 后仍无反应，排除 LDO 本身
- 代码层无法进一步定位，需硬件排查 P0.13 走线

### 最终决策：代码回退到 HEAD
- `git restore` 回退 `numpad.conf` + `numpad.overlay`（EXT_POWER/NFCT 改动）
- 工作区干净，代码状态 = HEAD `3ec4de0`（poll 三键 + 三层 keymap，无 ext-power）
- 重新编译通过（FLASH 261320B / 32.22%）
- **供电问题交由用户硬件层处理**（飞线 RAW 或外接 LDO），代码层不再尝试

### 当前可用状态
- 矩阵 + 编码器 + 三层 keymap 编译通过
- 飞线 RAW 供电时，AI32C 三键触摸触发音量增加（已验证）
- 固件 `build/zephyr/zmk.uf2`（522752 字节）

### 待办（硬件层，非代码）
- [x] ~~排查 P0.13 走线~~ / ~~外接 AMS1117-3.3~~ → **放弃维修（2026-08-29 定案）**：LDO 换新后仍无 3.3V（原因不明），飞线 BAT+/RAW 取电也已不可用，当前板子供电报废
- [ ] **换新 nice!nano 开发板**（新板到手烧录现有 zmk.uf2 即可，无需改代码）
- [ ] 电压安全：飞线 RAW 超 3.6V，需 3.3V 稳压（换板后此问题消失，新板走正常 VCC 排针）

## Session: 2026-08-29 — 全项目代码审查修复（9 处）

### 修复清单
1. **build-local.ps1（关键 bug）**：`-DZMK_EXTRA_MODULES=/workspace` → `/workspace/extra-module`。
   原路径无 `zephyr/module.yml`，Zephyr 模块扫描返回 None，extra-module 整体不进构建
   （AI32C 驱动/双 LED 模块/绑定全缺失）
2. **kscan_gpio_ai32c.c**：首个 poll 延迟 500ms，避开 AI32C 上电 400ms 自校准期
   （T_init，数据手册）未定义输出，防开机误报 KEY1
3. **kscan_gpio_ai32c.c**：新增深睡眠触摸唤醒——监听 `zmk_activity_state_changed`，
   SLEEP 前停轮询、两根 OUT 挂 `GPIO_INT_LEVEL_LOW`（nRF PORT/SENSE 路径在
   System Off 下有效）。空闲 OUT=11（高），任一触摸拉低即 DETECT 唤醒
   （唤醒=复位重启，开机后轮询 500ms 读回按住状态）
4. **kscan_gpio_ai32c.c**：编码表注释修正为 (OUT1,OUT2) 顺序，与代码 o1/o2 对齐
5. **kscan CMakeLists.txt**：include 路径 `../../` → `../../../`（三级目录少算一层，
   之前仅靠 Zephyr 头文件巧合未暴露；引入 zmk/activity.h 后暴露）
6. **ble_led.c**：新增 SLEEP 监听关灯——否则深睡眠时蓝灯 50% 概率停在常亮漏电
7. **battery_led.c**：三处边界——① SLEEP 前清灯（WS2812 锁存颜色会整晚亮）
   ② 拔 USB 时若 fn 层正显示电量则改显电量而非清屏 ③ 订阅
   `zmk_battery_state_changed`，真实电量到达后重渲染充电状态（防开机读到 0 误显示）
8. **numpad.overlay**：头部过时注释重写（单键→三键、编码器 R4C0→R2C3）；
   AI32C 节点删除误导性 `GPIO_ACTIVE_LOW|GPIO_PULL_UP` 死配置；编码器三重
   status 声明清理（overlay disabled + overlay okay + keymap okay → 默认 okay）
9. **文档一致性**：zmk.yml 去掉 OLED feature/description 改 AI32C+RGB；Kconfig.defconfig
   删除 SSD1306/I2C 死配置；binding yaml 描述改为三键+push-pull 说明

### Build result
- FLASH: 261844 B / 792 KB = **32.29%**（+0.07%）
- RAM: 63100 B / 256 KB = **24.07%**（不变）
- `zmk.uf2`: 523776 字节

### 遗留（未修，记录原因）
- 10ms 空闲轮询功耗：保持既有 poll 架构（项目约定），深睡眠已无轮询
- combo timeout 300ms 误触风险：低风险，保留
- CONFIG_ZMK_USB_LOGGING：硬件调试期保留，量产前再关
- "USB 供电=充电"启发式：无充电 IC 状态脚，硬件限制

### 待上机验证（换新开发板后）
- [ ] 深睡眠触摸唤醒
- [ ] 开机 500ms 内不误触
- [ ] 拔 USB 后 fn 层电量显示不再被清屏打断
- [ ] 深睡眠后电量灯/蓝牙灯确认为熄灭

## Session: 2026-08-29 — 收尾（git 推送 + gitignore）

- 提交 `6a5b251`（13 文件，+303/-51）已推送到 github.com:redbean-pie/KeyPad.git
- .gitignore 新增：`.firecrawl/`（Firecrawl 抓取的硬件资料缓存，纯调研用，
  结论已沉淀 docs/ 和 findings.md）、docs 下三个未定稿草稿
- **当前项目状态**：代码全部完成且已推送；等待换新 nice!nano 开发板后上机验证
