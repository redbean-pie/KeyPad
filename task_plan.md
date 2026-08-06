# Task Plan: Numpad ZMK 固件 — AI32C 触摸键集成

## Goal
将 AI32C 三通道电容触摸作为 3 个独立触摸键集成到 numpad 固件中，替代已移除的 OLED，实现触摸输入功能。已完成全部固件功能与硬件设计。

## Next Step
烧录 zmk.uf2 到 nice!nano v2，杜邦线模拟 AI32C 输出验证解码；PCB 打板后真实触摸测试。

## Current Phase
Phase 5

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
- [ ] 烧录测试三层切换 + 触摸功能
- [ ] **电压安全**：飞线 RAW(4.1V/5V) 超 nRF52840 3.6V 上限，需换 3.3V LDO 或修板载 LDO
- **Status:** in_progress（触摸功能已验证，三层 keymap 编译通过，待烧录实测 + 电压处理）

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

## Errors Encountered
| Error | Attempt | Resolution |
|-------|---------|------------|
| AI32C 本地 PDF 扫描图片无法 OCR | 1 | 从 xmxwdz.cn 下载文本版 PDF |
| szlcsc 页面 JS 渲染无法抓取 | 2 | firecrawl-search 找到替代源 |
| battery_led CMake zephyr_library_amend 失败 | 1 | 改 zephyr_library() 自建库 + 补 include |
| ble_led GPIO_DT_SPEC_GET 直接传参报错 | 1 | 改 static spec 变量取地址 |
| 滑条 sensor 方案死机 + 触摸无反应 | 1 | 废弃滑条方案，回退到 kscan 触摸键方案 |
| 短接 D6/D7 不触发触摸 | 多轮 | 根因：nice!nano 板载 3.3V LDO 损坏（输出 0V），AI32C 从未通电。飞线 RAW 供电后恢复 |
| pristine build 报 spi1_sleep lacks sensor-binding-cells | 1 | Zephyr 版本与 ZMK sensor binding 兼容性问题（非代码），用增量编译绕过 |

## Notes
- AI32C: 3通道电容触摸，2.5-5.5V，2线编码输出，内置消抖，7µA 睡眠
- **AI32C OUT 是 push-pull 输出**（非开漏），空闲 11，无需 PULL_UP
- 当前 FLASH 32.30% / RAM 24.10%（三层 keymap 待重新编译）
- 构建：`west build -d build`（增量，不要 pristine）/ `west build -s zmk/app -b nice_nano//zmk -d build -- -DSHIELD=numpad`（全量）
- **硬件修复**：nice!nano 板载 3.3V LDO 损坏，当前飞线 RAW->AI32C VDD 临时供电，待修 LDO 或加外接 AMS1117-3.3
- 所有 GPIO 已分配：左列 D2-D9 + 右列 D10-D18 + D19/D20/D21
- 历史：滑条 sensor 方案（ai32c_slider.c）于 2026-08-06 废弃，回退到 kscan 触摸键方案（kscan_gpio_ai32c.c）
