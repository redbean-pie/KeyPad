# Task Plan: Numpad ZMK 固件 — AI32C 触摸滑块集成

## Goal
将 AI32C 三通道电容触摸滑块集成到 numpad 固件中，替代已移除的 OLED，实现触摸输入功能。已完成全部固件功能与硬件设计。

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

### Phase 2: 固件配置
- [x] 编写 AI32C 自定义 kscan driver（中断+轮询混合 + PM 唤醒）
- [x] 创建 DT binding (`extra-module/dts/bindings/zmk,kscan-gpio-ai32c.yaml`)
- [x] 更新模块构建系统 (module.yml, CMakeLists.txt, Kconfig)
- [x] 更新 `numpad.overlay` 为 composite 结构（kscan_matrix + kscan_ai32c）
- [x] 更新 `numpad.keymap` 为 8 行布局，触摸键映射 C_MUTE/C_PREV/C_NEXT
- **Status:** complete

### Phase 3: 文档更新
- [x] 更新 `PINOUT.md` / `NOTES.md` / `PCB_DESIGN.md`
- [x] 更新 memory / 规划文件
- **Status:** complete

### Phase 4: 构建验证
- [x] 编译固件通过 ✅（FLASH 26.17% / RAM 19.05%）
- [x] 设备树节点正确生成
- **Status:** complete

### Phase 5: 烧录测试
- [ ] 烧录到 nice!nano v2，杜邦线模拟 AI32C 输出（D6/D7 碰 GND）验证解码
- [ ] PCB 打板后真实触摸测试
- [ ] 验证矩阵/编码器/RGB/蓝牙LED 正常
- **Status:** pending

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
4. ~~三个触摸通道映射？~~ ✅ C_MUTE / C_PREV / C_NEXT

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

## Errors Encountered
| Error | Attempt | Resolution |
|-------|---------|------------|
| AI32C 本地 PDF 扫描图片无法 OCR | 1 | 从 xmxwdz.cn 下载文本版 PDF |
| szlcsc 页面 JS 渲染无法抓取 | 2 | firecrawl-search 找到替代源 |
| battery_led CMake zephyr_library_amend 失败 | 1 | 改 zephyr_library() 自建库 + 补 include |
| ble_led GPIO_DT_SPEC_GET 直接传参报错 | 1 | 改 static spec 变量取地址 |

## Notes
- AI32C: 3通道电容触摸，2.5-5.5V，2线编码输出，内置消抖，7µA 睡眠
- 当前 FLASH 26.17% / RAM 19.05%
- 构建：`west build -s zmk/app -b nice_nano//zmk -d build -- -C zmk-cache.cmake -DSHIELD=numpad`
- 所有 GPIO 已分配：左列 D2-D9 + 右列 D10-D18 + D19/D20/D21
