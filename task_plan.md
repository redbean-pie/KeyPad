# Task Plan: Numpad ZMK 固件 — AI32C 触摸滑块集成

## Goal
将 AI32C 三通道电容触摸滑块集成到 numpad 固件中，替代已移除的 OLED，实现触摸输入功能。

## Next Step
烧录 zmk.uf2 到 nice!nano v2，焊接 AI32C 外围电路后测试触摸滑块三通道响应。

## Current Phase
Phase 4

## Phases

### Phase 1: AI32C 引脚确认
- [x] 从 xmxwdz.cn 下载 AI32C 文本版数据手册并完整提取
- [x] 确认 SOP-8 引脚：C1/K1/K2/K3/OUT2/OUT1/VDD/GND
- [x] 确认输出为 2 线二进制编码（OUT1+OUT2），非 3 路独立输出
- [x] 确认外围电路：4.7nF NPO@C1、100nF 退耦、VDD 串 20Ω、OUT 各 1K 上拉
- [x] 引脚信息已记录到 findings.md
- **Status:** complete

### Phase 2: 固件配置
- [x] 编写 AI32C 自定义 kscan driver (`extra-module/drivers/kscan/kscan_gpio_ai32c.c`)
- [x] 创建 DT binding (`extra-module/dts/bindings/zmk,kscan-gpio-ai32c.yaml`)
- [x] 更新模块构建系统 (module.yml, CMakeLists.txt, Kconfig)
- [x] 更新 `numpad.overlay` 为 composite 结构（kscan_matrix + kscan_ai32c）
- [x] 更新 `numpad.keymap` 为 9 行布局，触摸键映射 C_MUTE/C_PREV/C_NEXT
- [x] 驱动架构：2 GPIO 轮询 (10ms) → 2-bit 解码 → 3 个独立 key 事件
- **Status:** complete

### Phase 3: 文档更新
- [x] 更新 `PINOUT.md` 中 AI32C 接线说明（去除"待确认"标记）
- [x] 更新 `NOTES.md` 中 AI32C 相关描述
- [x] 更新 memory 中的进度状态
- **Status:** complete

### Phase 4: 构建验证
- [x] 编译固件通过 ✅
- [x] FLASH: 24.87% (+0.14%), RAM: 18.50% (+0.09%)
- [x] 设备树节点正确生成 (composite + ai32c)
- [x] AI32C driver .obj 已链接
- **Status:** complete

### Phase 5: 烧录测试
- [ ] 烧录到 nice!nano v2
- [ ] 测试触摸滑块三通道响应
- [ ] 验证矩阵/编码器仍正常工作
- **Status:** pending

## Key Questions
1. ~~AI32C SOP-8 各脚位编号和功能是什么？~~ ✅ 已确认
2. ~~AI32C 触摸触发后输出什么信号？~~ ✅ 2 线二进制编码，低有效
3. ZMK 如何处理二进制编码的 GPIO 输入作为 3 个独立按键？需要写自定义 kscan driver
4. 三个触摸通道分别映射到什么按键功能？（静音 / 播放暂停 / 切层？）

## Decisions Made
| Decision | Rationale |
|----------|-----------|
| 沿用 nice!nano 开发板方案（不画整板）| 快速迭代，整板设计转为备用 |
| AI32C 替换 OLED | OLED 已完全移除，释放 I2C 引脚给触摸 |
| **D2→OUT1, D3→OUT2（仅需 2 GPIO）** | AI32C 为 2 线二进制编码输出，非 3 路独立 |
| **D1 释放** | 原分配给 T3，实际 AI32C 无独立 T3 输出 |
| 编码器按下切层（默认层↔亮度层）| 双模式：音量± / 亮度± |
| AI32C 需自定义 kscan driver | ZMK 无内置二进制解码 kscan，需自写 |

## Errors Encountered
| Error | Attempt | Resolution |
|-------|---------|------------|
| AI32C 本地 PDF 扫描图片无法 OCR | 1 | 从 xmxwdz.cn 下载文本版 PDF，成功提取 |
| `pdftoppm` 未安装无法渲染 PDF | 1 | pypdf 直接提取文本版 PDF |
| Firecrawl/WebFetch 无法抓取 szlcsc 页面 | 2 | curl 后确认 JS 渲染页面，换用 firecrawl-search 找到替代 PDF |

## Notes
- AI32C: SAM&WING 芯网，SOP-8，3通道电容触摸，2.5-5.5V，2线编码输出
- 外围：C1=4.7nF NPO、VDD 串 20Ω（不可省）、100nF 退耦、OUT1/OUT2 各 1K 上拉
- 低有效输出：无触摸=11，KEY1=00，KEY2=01，KEY3=10
- AI32C 特有：内置消抖、300ms 初始化、7µA 睡眠电流
- 构建命令：`west build -s zmk/app -b nice_nano//zmk -d build -- -C zmk-cache.cmake -DSHIELD=numpad`
- 增量：`west build -d build`
- 当前 FLASH 39.94% / RAM 21.34%
