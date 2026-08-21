# AI Terminal 研究综合报告（工作稿）

**作者：Manus AI**  
**范围：** Windows-first、面向 AI Coding / Coding Agent 工作流的现代终端。本文是技术选型前的研究综合；最终架构文档将把结论落实为可验证的设计与实现边界。

## 研究结论摘要

传统终端竞争的核心已经从“能否启动 shell”转为**VT/xterm 兼容性、输入延迟、长输出滚动、渲染吞吐、会话组织和平台整合**。AI Coding CLI 又增加了一个不同的管理平面：任务可持续数十分钟、会修改项目文件并运行命令、会在权限与用户输入点暂停、还可能派生子 Agent。因此，本产品不应把 Agent UI 建立在脆弱的 stdout 正则猜测上，而应采用“**真实终端保持原样 + 可选、可信的 Agent 适配器叠加元数据**”的双平面方案。[1] [2]

| 研究判断 | 结论 | 对 AI Terminal 的约束 |
| --- | --- | --- |
| 终端核心 | ConPTY 只提供 PTY 与管道，不提供屏幕渲染；宿主负责 VT 输入输出、窗口尺寸、生命周期和 UI。 | 必须分离 `PtySession`、`TerminalCore`、`Renderer`、`Shell/Agent Adapter`。不允许 Web UI 直接读取 stdout。 |
| 热路径 | PTY 读取、VT 解析、滚动、字形绘制是独立的高吞吐路径。 | 使用有界队列、增量 parser、脏区绘制、渲染线程与有限 scrollback；禁止 React/JS 每行 setState。 |
| Agent 语义 | “正在运行”“等待权限”“等待输入”“完成”“失败”是 CLI 真实交互的一部分。 | 状态机必须是独立域模型；优先使用 hooks/协议/明确启动器，output pattern 仅作低置信度降级。 |
| Windows 产品体验 | 该产品需要 Mica/Fluent、IME、Toast、Taskbar、Tray、Credential Manager 和高 DPI 支持。 | 生产 UI 应选择 Windows 原生壳层，而不是以 Electron 作为默认方案。 |
| 长期开放性 | Terminal core、Agent adapter、工作区、插件、通知都将演化。 | 核心进程以版本化 IPC 和能力接口隔离；插件不直接获得 PTY 句柄、任意进程或 UI DOM。 |

## 主流终端对比

| 产品 | 可借鉴点 | 主要局限或不采用原因 | 设计吸收 |
| --- | --- | --- | --- |
| Windows Terminal | Windows 原生 ConPTY、VT 生态、设置/Profile、PowerShell/CMD/WSL 基线；开源项目含 DirectWrite 文本布局/渲染、UTF 文本缓冲和 VT 组件。[3] | 不以 AI Agent 为中心；大型 C++ 代码库不宜直接嵌入 MVP。 | 将其作为 Windows 兼容性与行为测试基线，不作为直接代码依赖。 |
| Warp | Rust、GPU 渲染、命令块、可编辑输入区和现代操作体验；其 block 需要 shell 深度集成。[4] | 自建 GPU UI 的研发成本与风险很高；强 block 语义可能破坏 TUI 兼容。 | 采用“可选 shell integration + block 元数据”，不重写传统终端输入。 |
| Ghostty | 共享核心 `libghostty` 与原生 GUI 分离，强调核心/GUI 的清晰边界和多线程模型。[5] | 当前公共核心 API 并非稳定嵌入契约，Windows 路径不应依赖未稳定 API。 | 采用可替换终端核心与原生 Shell 的分层。 |
| WezTerm | 以 domain 组织 window/tab/pane，本地/SSH/Unix/TLS 复用会话模型清晰。[6] | 远端持久 multiplex 需要专用服务与兼容远端组件；WSL 2 AF_UNIX interop 有限制。 | 工作区域模型为 `Workspace → Tab Tree → Pane → Session`；远程可恢复留到后续。 |
| Alacritty | Rust、跨平台、`alacritty_terminal` 独立 crate、Apache-2.0/MIT 许可路径。[7] | 产品整体是完整终端而非稳定 SDK；不直接迁入其 renderer/UI。 | 作为 VT 解析/状态机的首选技术验证候选，锁版本并做许可审查。 |
| Kitty | GPU/SIMD、线程化渲染、shell integration、会话、可组合工具与协议扩展。[8] | 产品和扩展协议的取舍偏 Unix；不是 Windows UI/ConPTY 解决方案。 | 增强能力以可选协议加入，不能污染 VT 基线。 |
| Tabby | Profile、SSH/serial、分屏、持久 Tab、主题、WSL/CMD/PowerShell 覆盖广，插件模块丰富。[9] | Electron 模型与目标内存、冷启动和热路径要求冲突。 | 借鉴连接器/插件/Settings 信息架构，不复用框架。 |
| Hyper | Electron + xterm.js + Node 扩展，主题和社区插件体验成熟。[10] | 插件可触达渲染与 Node 生态，存在性能、供应链和能力边界风险。 | 用受限、版本化 Host API 替代直接 UI/进程访问。 |
| iTerm2 | Shell integration 可显式报告 cwd、命令边界和退出码；也提供 prompt trigger 降级。[11] | macOS 专用，触发器解析并非可靠协议。 | 采集来源按“协议 > hook > prompt 解析 > 未知”分级。 |
| Rio | Rust/GPU 路线、Windows MSI/portable 发布，适合观察跨平台打包与渲染实践。[12] | 嵌入式稳定 API、复杂工作区/Agent 管理并非其定位。 | 不直接复用整机代码；作为性能与发行方式参考。 |
| Wave | Workspace 管理、AI/Claude Code 集成、可持久 Tab/布局/历史与连接能力。[13] | 接近 all-in-one 工作台，容易滑向 IDE 范围。 | 采纳临时/保存工作区的明确模型，同时坚持 Terminal First。 |

## AI Coding CLI 的共性与差异

AI Coding CLI 的关键不同不在于“输出更多”，而在于它们把**项目访问边界、命令执行、写文件、权限批准、子 Agent 与任务恢复**带进了终端会话。Claude Code 的权限规则可细分到工具/命令并可用 hooks 扩展；Codex 将沙箱边界与审批策略分开；Gemini CLI 对写文件和 shell 命令的变更型工具要求确认；OpenCode 和 Copilot CLI 都支持可配置/可委派的子 Agent。[14] [15] [16] [17] [18]

| 工作流特征 | 普通终端 | AI Coding 终端需要的补强 |
| --- | --- | --- |
| 执行时长 | 大多是短命令、服务或 TUI。 | 展示持续时间、最近活动、运行阶段、后台/前台状态。 |
| 用户动作 | 输入命令、读取输出。 | 除输入外，还会授权命令、批准写入、回答问题、确认计划。 |
| 上下文 | 当前 shell、cwd、环境变量。 | 项目根目录、Git 状态、Agent 类型、权限模式、子 Agent、任务意图。 |
| 多任务 | 标签/分屏即可。 | 聚合多个 Agent，按“正在运行 / 等待我 / 已结束 / 失败”排序并支持非打扰提醒。 |
| 可观测性 | 进程结束码通常足够。 | 需可靠区分“输出安静但仍在思考”和“等待输入/权限”；记录证据来源和置信度。 |
| 隐私 | 可保存 scrollback。 | 默认 Metadata Only；终端全文、提示词、路径、令牌片段均可能敏感。 |

Aider 代表 Git-first 终端结对编程，包含仓库地图、脚本与通知；Copilot CLI 则可将复杂工作委派给 Explore、Task、Review、Research 等子 Agent。这要求 AI Terminal 的数据模型表达父会话、子 Agent、项目边界与 Git 只读摘要，而不是仅把每个进程当作相同的 Tab。[19] [20]

## 底层 Windows 发现

`CreatePseudoConsole` 接受用户输入管道、应用输出管道和字符尺寸。当前 I/O 是同步管道；终端宿主负责呈现输出并把用户输入序列化到输入流。关闭时 ConPTY 会向客户端发送 `CTRL_CLOSE_EVENT`，旧 Windows 版本在未关闭/排空输出时可能阻塞，因此关闭逻辑不能位于输出 reader 线程。[21] [22]

> “Closing a pseudoconsole will send CTRL_CLOSE_EVENT to each client application that is still connected.” — Microsoft Learn [22]

这意味着生产实现须为每个 Session 配置**独立的 PTY I/O 线程、进程等待器、关闭状态机和缓冲队列**。任何一个 session 的挂起、崩溃、巨量输出或关闭超时都不得阻塞窗口 UI 或其他 Tab。

## 安全与隐私发现

AI Coding CLI 的权限状态必须由原工具安全模型决定。终端可以显示状态、提醒和命令/文件摘要，但不应在没有原工具明确接口的情况下代替 CLI 批准命令。Claude Code、Codex、Gemini CLI 和 OpenCode 都把权限与项目边界视为基本语义。[14] [15] [16] [17]

在传统终端层，默认开启**多行粘贴**和**大粘贴**警告；多行粘贴可能立即作为多条 shell 命令执行。超链接/OSC 8 应显示真实 URI 并要求修饰键点击；剪贴板、外部 scheme、目录标题和 OSC payload 需在 parser 层限长、过滤和权限控制。[23]

## 设计原则（研究后的冻结结论）

1. **Terminal First。** 主窗口首先是高兼容的 terminal emulator，不是代码编辑器或聊天应用。
2. **双平面。** VT/PTy 主平面永远可用；Agent Dashboard、状态、Git、命令块是可选控制平面。
3. **先证据、后状态。** Agent adapter 必须保留状态来源和置信度；无法确认时显示 `Running (unclassified)`，不假装知道。
4. **Windows 原生外壳。** Toast、Tray、Taskbar、Credential Manager、IME 与无障碍能力属于产品，不是插件。
5. **进程隔离。** UI、Terminal Service、插件 host 的故障域分开；一条会话的堵塞不可带崩全局。
6. **默认最小保存。** Session history 默认仅元数据；敏感全文记录须显式启用、加密并支持擦除。
7. **增强而不破坏兼容性。** TUI、vim、top、ssh、python REPL、鼠标模式、alternate screen 走标准终端路径；Command Block 只在协议明确时出现。

## 参考文献

[1]: https://devblogs.microsoft.com/commandline/windows-command-line-introducing-the-windows-pseudo-console-conpty/ "Microsoft: Introducing ConPTY"
[2]: https://code.claude.com/docs/en/permissions "Claude Code permissions"
[3]: https://github.com/microsoft/terminal "microsoft/terminal"
[4]: https://www.warp.dev/blog/how-warp-works "Warp: How Warp Works"
[5]: https://ghostty.org/docs/about "Ghostty: About"
[6]: https://wezterm.org/multiplexing.html "WezTerm Multiplexing"
[7]: https://github.com/alacritty/alacritty "Alacritty"
[8]: https://sw.kovidgoyal.net/kitty/ "kitty"
[9]: https://github.com/eugeny/tabby "Tabby"
[10]: https://hyper.is/ "Hyper"
[11]: https://iterm2.com/documentation-shell-integration.html "iTerm2 Shell Integration"
[12]: https://rioterm.com/ "Rio Terminal"
[13]: https://docs.waveterm.dev/workspaces "Wave Workspaces"
[14]: https://code.claude.com/docs/en/permissions "Claude Code permissions"
[15]: https://learn.chatgpt.com/docs/sandboxing "Codex sandboxing"
[16]: https://geminicli.com/docs/reference/tools/ "Gemini CLI tools"
[17]: https://opencode.ai/docs/agents/ "OpenCode agents"
[18]: https://docs.github.com/en/copilot/how-tos/copilot-cli/use-copilot-cli/overview "GitHub Copilot CLI"
[19]: https://aider.chat/docs/ "Aider documentation"
[20]: https://docs.github.com/en/copilot/how-tos/copilot-cli/use-copilot-cli/overview "GitHub Copilot CLI"
[21]: https://learn.microsoft.com/en-us/windows/console/createpseudoconsole "CreatePseudoConsole"
[22]: https://learn.microsoft.com/en-us/windows/console/closepseudoconsole "ClosePseudoConsole"
[23]: https://learn.microsoft.com/en-us/windows/terminal/customize-settings/interaction "Windows Terminal interaction settings"
