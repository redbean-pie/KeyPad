# Task Plan: Numpad ZMK 固件 — AI32C 触摸键集成

## Goal
将 AI32C 三通道电容触摸作为 3 个独立触摸键集成到 numpad 固件中，替代已移除的 OLED，实现触摸输入功能。已完成全部固件功能与硬件设计。

## Next Step
换新 nice!nano 开发板（板载供电报废：P0.13/CE 拉不高 + LDO 换新仍无 3.3V + 飞线取电失效）。新板到手后：烧录 zmk.uf2 → 直接上机验证 Phase 6 修复项（深睡眠触摸唤醒、开机无误触、深睡眠 LED 熄灭）→ 之后 PCB 打板真实触摸测试。

## Current Phase
Phase 6（代码审查修复完成，待换新开发板后上机验证）

## Phases

### Phase 1: AI32C 引脚确认
- [x] 从 xmxwdz.cn 下载 AI32C 文本版数据手册并完整提取
- [x] 确认 SOP-8 引脚：C1/K1/K2/K3/OUT2/OUT1/VDD/GND
- [x] 确认输出为 2 线二进制编码（OUT1+OUT2），非 3 路独立输出
- [x] 确认外围电路：4.7nF NPO@C1、100nF 退耦、VDD 串 20Ω、OUT 各 1K 上拉
- [x] 引脚信息已记录到 findings.md
- **Status:** complete

### Phase 2: 固件配置（kscan 触摸键方案）
- [x] 编写 AI32C 自定义 kscan driver（中断+轮询混合 + PM 唤醒）
- [x] 创建 DT binding (`extra-module/dts/bindings/zmk,kscan-gpio-ai32c.yaml`)
- [x] 更新模块构建系统 (module.yml, CMakeLists.txt, Kconfig)
- [x] 更新 `numpad.overlay` 为 composite 结构（kscan_matrix + kscan_ai32c）
- [x] 更新 `numpad.keymap` 为 8 行布局
- [x] 触摸键映射：音量层 T1=C_PREV T2=C_MUTE T3=C_NEXT；亮度层 T1=C_BRI_UP T2=none T3=C_BRI_DN
- **Status:** complete

### Phase 3: 文档更新
- [x] 更新 `PINOUT.md` / `NOTES.md` / `PCB_DESIGN.md`
- [x] 更新 memory / 规划文件
- **Status:** complete

### Phase 4: 构建验证
- [x] 编译固件通过 ✅（FLASH 32.30% / RAM 24.10%）
- [x] 设备树节点正确生成
- **Status:** complete

### Phase 5: 烧录测试
- [x] 矩阵按键 + 编码器 + Combo 通过
- [x] AI32C 触摸功能验证（飞线 RAW 供电后，三键触发音量增加）
- [x] 编译通过三层 keymap（2026-08-06 修复 4 个阻塞后全量编译通过，FLASH 32.22% / RAM 24.07%）
- [x] 尝试 EXT_POWER + NFCT_PINS_AS_GPIOS 拉高 P0.13 修复供电（失败，已回退）
- [x] ~~供电修复（硬件层）~~ → **结论（2026-08-29）：板载供电判定报废**。P0.13 拉不高 CE 无解，LDO 换新后仍无 3.3V 输出（原因不明），且飞线 BAT+ 取电也已不可用。方案：**换新的 nice!nano 开发板**
- **Status:** closed（硬件层放弃维修，换板解决）

### Phase 6: 全项目代码审查修复（2026-08-29）
- [x] build-local.ps1：ZMK_EXTRA_MODULES 路径错误（/workspace → /workspace/extra-module，原路径整个模块不进构建）
- [x] AI32C 驱动：首 poll 延迟 500ms（避开上电 400ms 自校准期，防开机误报 KEY1）
- [x] AI32C 驱动：深睡眠触摸唤醒（监听 activity_state_changed，SLEEP 前挂 LEVEL_LOW SENSE 中断）
- [x] AI32C 驱动：编码表注释修正为 (OUT1,OUT2) 顺序
- [x] kscan CMakeLists：include 相对路径 ../../ → ../../../（三级目录层级错误）
- [x] ble_led：深睡前关灯（防常亮漏电）
- [x] battery_led：深睡前清灯 + 拔 USB 保留 fn 层电量显示 + 订阅电量事件防开机误显示
- [x] overlay/keymap：头部过时注释、AI32C 死配置 GPIO flags、编码器三重 status 清理
- [x] 文档一致性：zmk.yml（去 OLED）、Kconfig.defconfig（去 SSD1306）、binding yaml（三键说明）
- [x] 编译通过：FLASH 32.29% / RAM 24.07%，zmk.uf2 523776 字节
- [ ] 上机验证：深睡眠触摸唤醒、开机无误触、拔 USB 显示、深睡眠 LED 熄灭
- **Status:** in_progress（代码完成并编译通过，上机验证待硬件供电修复）

### 额外功能（已全部完成）
- [x] **WS2812 RGB**：主链 19 颗（D20/SPIM3）+ 电量链 2 颗（D19/SPIM2）
- [x] **电量/充电显示**：`extra-module/battery_led`，进 fn 层显示电量；USB 供电时橙闪充电/绿充满
- [x] **蓝牙状态 LED**：D21 直驱蓝灯，`extra-module/ble_led`，已连接常亮/未连接闪烁
- [x] **低功耗**：Idle 2min 关灯 + Deep Sleep 15min 断电（触摸唤醒）
- [x] **电源开关**：DPDT（MSS22D18G2）正负极双断

## Key Questions
1. ~~AI32C SOP-8 各脚位编号和功能是什么？~~ ✅ 已确认
2. ~~AI32C 触摸触发后输出什么信号？~~ ✅ 2 线二进制编码，低有效
3. ~~ZMK 如何处理二进制编码输入？~~ ✅ 自定义 kscan driver
4. ~~三个触摸通道映射？~~ ✅ T1=上一首 T2=静音 T3=下一首（音量层）/ T1=亮度+ T2=无 T3=亮度-（亮度层）

## Decisions Made
| Decision | Rationale |
|----------|-----------|
| 沿用 nice!nano 开发板方案（不画整板）| 快速迭代，整板设计转为备用 |
| AI32C 替换 OLED | OLED 已完全移除，释放 I2C 引脚 |
| D6→OUT1, D7→OUT2（仅 2 GPIO）| AI32C 为 2 线编码输出，非 3 路独立 |
| 引脚按排针物理位置连续分配 | 布线最短、接线表直观 |
| 编码器按压复用 R4C0 | 0 键 2U 宽空位，矩阵 6×4 缩为 5×4 |
| AI32C 中断+轮询混合驱动 | 空闲无空转省电 + PM 深睡眠触摸唤醒 |
| WS2812 主链/电量链独立 | rgb_underglow 50ms 覆盖整链，需独立避免冲突 |
| 蓝牙 LED 直驱 D21 | 独立单色灯，不占电量灯 |
| 低功耗 Idle+Deep Sleep | AI32C 自睡眠 7µA + ZMK 自动待机 |
| 电源开关 DPDT 正负极双断 | 彻底断电；USB 供电独立不受影响 |
| 移除拨码开关 | 自动省电机制已足够 |
| **滑条 sensor 方案废弃，回退 kscan 触摸键**（2026-08-06）| 滑条方案死机 + 完全无反应：holding 20Hz 触发堵塞 behavior queue；依赖 ZMK legacy compat 路径（标记 REMOVE ME）未来会失效；PM RESUME 死锁边界情况 |
| 触摸键切层映射 | 编码器按压负责切层，触摸键在每层做不同功能（媒体/亮度）|
| **驱动改 poll 三键独立版**（2026-08-06）| GPIOTE 中断不触发（根因是 LDO 损坏没通电，但 poll 更可靠且已验证）|
| **AI32C 纯输入无 PULL_UP**（2026-08-06）| AI32C OUT 是 push-pull 输出，主动驱动高低，无需 PULL_UP；飞线 5V 时 PULL_UP 会冲突 |
| **EXT_POWER 修复供电失败，代码回退 HEAD**（2026-08-08）| 根因纠正：LDO 没坏，是 CE=0V（P0.13 未拉高）关闭 LDO。加 EXT_POWER 节点 + CONFIG_NFCT_PINS_AS_GPIOS + 驱动手动拉高 P0.13，烧录后 CE 仍 0V。代码层无法修复，回退到 HEAD 稳定版，供电交硬件层处理 |
| **深睡眠唤醒改为活动事件 + SENSE 中断**（2026-08-29）| ZMK 深睡眠=sys_poweroff（复位重启式唤醒），PM_DEVICE action 不适合本驱动；监听 zmk_activity_state_changed 在 SLEEP 前挂 LEVEL_LOW 中断最贴合现有 poll 架构 |
| **有意保留项**（2026-08-29）| 10ms 空闲轮询（poll 架构约定，深睡眠已无轮询）；combo 300ms（低风险）；USB 日志（调试期保留）；USB 供电=充电启发式（无充电 IC 状态脚）|

## Errors Encountered
| Error | Attempt | Resolution |
|-------|---------|------------|
| AI32C 本地 PDF 扫描图片无法 OCR | 1 | 从 xmxwdz.cn 下载文本版 PDF |
| szlcsc 页面 JS 渲染无法抓取 | 2 | firecrawl-search 找到替代源 |
| battery_led CMake zephyr_library_amend 失败 | 1 | 改 zephyr_library() 自建库 + 补 include |
| ble_led GPIO_DT_SPEC_GET 直接传参报错 | 1 | 改 static spec 变量取地址 |
| 滑条 sensor 方案死机 + 触摸无反应 | 1 | 废弃滑条方案，回退到 kscan 触摸键方案 |
| 短接 D6/D7 不触发触摸 | 多轮 | 根因：LDO 的 CE=0V（P0.13 未拉高）关闭 LDO，AI32C VDD=0V 从未通电。飞线 RAW 供电后恢复 |
| pristine build 报 spi1_sleep lacks sensor-binding-cells | 1 | Zephyr 版本与 ZMK sensor binding 兼容性问题（非代码），用增量编译绕过 |
| EXT_POWER 拉高 P0.13 烧录后 CE 仍 0V | 1 | 加 CONFIG_NFCT_PINS_AS_GPIOS + 驱动手动拉高仍无效，P0.13 走线/硬件问题，代码层放弃，回退 HEAD |
| kscan 驱动引入 zmk/activity.h 报 No such file | 1 | kscan CMakeLists include 相对路径少算一层（../../ → ../../../），暴露了本就错误的路径 |

## Notes
- AI32C: 3通道电容触摸，2.5-5.5V，2线编码输出，内置消抖，7µA 睡眠
- **AI32C OUT 是 push-pull 输出**（非开漏），空闲 11，无需 PULL_UP
- 当前 FLASH 32.29% / RAM 24.07%（2026-08-29 Phase 6 修复后，zmk.uf2 523776 字节）
- 构建：`west build -d build`（增量，不要 pristine）/ `west build -s zmk/app -b nice_nano//zmk -d build -- -DSHIELD=numpad`（全量）
- **硬件结论（2026-08-29 定案）**：当前 nice!nano 板载供电报废——P0.13 拉不高 CE 无解，LDO（ME6211C33M5G-N）换新后仍无 3.3V 输出（原因不明），飞线 RAW/BAT+ 取电也已不可用。不维修，换新开发板解决。新板到手直接烧录现有 zmk.uf2 验证，无需改代码
- **代码状态**：工作区含 2026-08-29 Phase 6 修复（9 项，未提交）——基于 HEAD `3ec4de0`，poll 三键 + 三层 keymap + 深睡眠唤醒/关灯修复，固件 `build/zephyr/zmk.uf2`（523776 字节）已编译生成
- **上机验证清单**（换新板后）：深睡眠触摸唤醒、开机 500ms 无误触、拔 USB fn 层电量不被清屏、深睡眠后电量灯/蓝牙灯熄灭
- 所有 GPIO 已分配：左列 D2-D9 + 右列 D10-D18 + D19/D20/D21
- 历史：滑条 sensor 方案（ai32c_slider.c）于 2026-08-06 废弃，回退到 kscan 触摸键方案（kscan_gpio_ai32c.c）
