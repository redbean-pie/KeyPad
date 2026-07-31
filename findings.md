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
| 5 | OUT2 | 数据通道 2 输出 | D3 / P0.20（1K 上拉至 VDD） |
| 6 | OUT1 | 数据通道 1 输出 | D2 / P0.17（1K 上拉至 VDD） |
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

## 引脚分配决策
| 信号 | Pro Micro | nRF52840 | 说明 |
|------|-----------|----------|------|
| OUT1 | D2 | P0.17 | AI32C 数据通道 1 |
| OUT2 | D3 | P0.20 | AI32C 数据通道 2 |
| D1 | — | P0.6 | **释放**，可作其他用途 |

原计划 D1→T3 是错误的——AI32C 没有独立 T3 输出，3 个通道通过 2 根线编码。

## Technical Decisions
| Decision | Rationale |
|----------|-----------|
| AI32C 替代 OLED 而非共存 | OLED 已完全移除，释放 I2C 引脚 |
| D2→OUT1, D3→OUT2（仅 2 GPIO）| AI32C 为 2 线编码输出，非 3 路独立 |
| 需自定义 ZMK kscan driver | ZMK 无内置二进制解码 kscan |
| 暂不启用深睡眠 | 等触摸功能稳定后再考虑 |
| nice!nano 3.3V 直接供电 AI32C | 在 2.5-5.5V 范围内 |

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
