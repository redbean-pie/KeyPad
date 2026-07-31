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

### Phase 5: 烧录测试

- **Status:** pending
- **阻塞原因：** PCB 未打板，无法连接 AI32C 硬件
- **可做验证（无 PCB）：**
  - 烧录固件后验证矩阵/编码器仍正常工作（证明 composite + driver 不崩溃）
  - 杜邦线将 D6/D7 碰 GND 模拟触摸输出，验证驱动解码逻辑
  - RGB 灯珠接线后验证发光

## Test Results
| Test | Input | Expected | Actual | Status |
|------|-------|----------|--------|--------|
| 编译 | `west build -d build` | 编译成功 | FLASH 39.94%, RAM 21.34% | ✓ |
| 矩阵按键 | 按下物理按键 | USB HID 输出对应键码 | 通过 | ✓ |
| 编码器音量 | 旋转 EC11（默认层）| 系统音量 ± | 通过 | ✓ |
| 编码器亮度 | 按下切层后旋转 | 屏幕亮度 ± | 通过 | ✓ |
| Combo bootloader | NumLock + - 同时按 | 进入 bootloader | 通过 | ✓ |
| Combo reset | NumLock + * 同时按 | 系统重启 | 通过 | ✓ |
| AI32C 触摸 | 触摸滑块 | 待定 | 待定 | ⬜ |

## Error Log
| Timestamp | Error | Attempt | Resolution |
|-----------|-------|---------|------------|
| 2026-07-31 | AI32C 本地 PDF 扫描图片无法 OCR | 1 | 从 xmxwdz.cn 下载文本版 PDF |
| 2026-07-31 | szlcsc 页面 JS 渲染，Firecrawl/WebFetch 均失败 | 1 | firecrawl-search 找到替代源 |
| 2026-07-31 | AI32C 引脚分配错误（误认为 3 路独立输出）| — | 数据手册确认为 2 线编码，修正为 2 GPIO |

## 5-Question Reboot Check
| Question | Answer |
|----------|--------|
| Where am I? | Phase 5 — 烧录测试（待 PCB 打板） |
| Where am I going? | Phase 5: 烧录 → 杜邦线模拟测试 → PCB 打板后真实测试 |
| What's the goal? | 将 AI32C 触摸滑块集成到 numpad 固件 |
| What have I learned? | AI32C 全规格已确认，driver 已写完编译通过，待硬件验证 |
| What have I done? | Phase 1-4 全部完成：引脚确认 → driver编写 → 文档更新 → 编译通过 |
