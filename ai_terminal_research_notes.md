# AI Terminal 研究笔记（工作稿）

## 已核验的底层终端与架构结论

| 主题 | 核验结论 | 对 AI Terminal 的设计含义 | 来源 |
| --- | --- | --- | --- |
| Windows ConPTY | ConPTY 提供 Windows 终端宿主与命令行程序之间的伪控制台通道，数据以异步方式收发；宿主需要通过输入/输出管道连接并处理 VT 序列。 | 应将 ConPTY 置于独立 PTY/会话层，UI 不直接读取进程 stdout；需要明确处理调整尺寸、VT 输入/输出和进程生命周期。 | Microsoft, 2018 [1] |
| Ghostty 分层 | Ghostty 将终端仿真、字体和渲染置于共享的 `libghostty` 核心，原生 GUI 作为消费者，从而分离核心与平台 UI。 | AI Terminal 应采用“可替换终端核心 + Windows 原生壳层 + 独立会话服务”的边界，避免把仿真逻辑耦合进 UI。 | Ghostty Docs [2] |
| 原生交互 | Ghostty 使用平台原生的标签、分屏、快捷键和系统能力，并按平台惯例提供快捷键。 | 对 Windows-first 产品，优先采用 WinUI 3/Windows App SDK 的窗口、托盘、Toast、Credential Manager 与 IME 支持，而不是自绘所有非终端控件。 | Ghostty Docs [2] |

## 研究参考

[1]: https://devblogs.microsoft.com/commandline/windows-command-line-introducing-the-windows-pseudo-console-conpty/ "Windows Command-Line: Introducing the Windows Pseudo Console (ConPTY)"
[2]: https://ghostty.org/docs/about "About Ghostty"

> 此文件是过程记录；最终交付会重新组织为可引用的正式架构文档。


| 会话多路复用 | WezTerm 将窗口、标签和 Pane 组织在可连接的 multiplexing domain 下；本地 UI 可附着到域中的会话，同时提供本地、SSH、Unix 和 TLS 域。 | MVP 中“窗口—工作区—Tab—Pane—会话”的结构应独立于 UI；远端持久会话/重新附着应作为后续 Server/Domain 扩展，而不是让 UI 进程拥有全部会话。 | WezTerm Docs [3] |
| WSL 与远端限制 | WezTerm 文档说明 WSL 1 可通过 Windows 上的 Unix socket 连入，WSL 2 不支持 AF_UNIX interop；远端 multiplex SSH domain 需要远端安装兼容的 wezterm。 | AI Terminal 的 MVP 应以独立的 `wsl.exe`/ConPTY 会话为主，不承诺跨崩溃“附着”到任意 WSL 进程；后续专用会话服务器才解决持久/远程复连。 | WezTerm Docs [3] |
| Windows Terminal 开源边界 | Microsoft 的 Terminal 仓库公开包含 Windows Terminal、console host 与相关组件，仓库采用 MIT 许可；其介绍列出 DirectWrite 文本布局/渲染、UTF-16/UTF-8 文本缓冲区和 VT parser/emitter。 | 可借鉴其模块划分与协议测试思路，但不应在 MVP 直接迁入大型 C++ 核心；需基于逐文件许可与维护成本审查后才可复用代码。 | Microsoft Terminal [4] |

[3]: https://wezterm.org/multiplexing.html "WezTerm Multiplexing"
[4]: https://github.com/microsoft/terminal "microsoft/terminal repository"


| Claude Code 权限 | Claude Code 将 Bash、写文件和网络请求等操作纳入分级权限；它能在工作中调整规则，规则由 CLI 强制执行，而不只是提示词约束。 | Agent 适配器应优先读取明确的可观测信号（hooks、协议或退出状态）；“等待授权”必须被建模为一等状态，并显示命令/文件范围，不得由终端代替 CLI 批准。 | Claude Code Docs [5] |
| Codex 沙箱与审批 | Codex 将“沙箱边界”和“审批策略”分开；超出文件/网络边界时会暂停请求审批，Windows 使用原生 Windows sandbox，WSL2 使用 Linux 实现。 | UI 要分别呈现：执行中、处于受限沙箱、等待审批；审批操作仍应在所属 CLI 的交互流程中完成。产品层绝不能把“检测到输出”误作安全授权。 | OpenAI Docs [6] |
| Agent 安全模型 | 两种 CLI 都将项目/工作区边界作为权限语义的重要部分。 | Workspace 建立时必须显示并记录根目录、信任状态、Agent 启动参数及权限模式；不应在未确认情况下扩大目录、网络或权限范围。 | Claude Code Docs [5] [6] |

[5]: https://code.claude.com/docs/en/permissions "Claude Code permissions"
[6]: https://learn.chatgpt.com/docs/sandboxing "Codex sandboxing"


| Gemini CLI 确认 | Gemini CLI 对修改文件或执行 shell 命令等 mutating tools 要求人工确认，并在确认前展示 diff 或准确命令；同时具备沙箱和 trusted folders 概念。 | 应把 `WaitingPermission` 作为跨 Agent 的规范化状态，优先展示 Agent 自己给出的 diff/命令摘要；终端仅做可见性与提醒，不截获或替换确认流程。 | Gemini CLI Docs [7] |
| OpenCode Agent 模型 | OpenCode 可配置主 Agent 与子 Agent，并以 `ask`、`allow`、`deny` 粒度管理读写、bash、外部目录、网络、MCP 等工具权限。 | Adapter 接口应支持“当前子 Agent/模式”“工具权限策略”“等待问题/批准”等能力字段；面板可展示结构化状态，传统 PTY 保持原样。 | OpenCode Docs [8] |

[7]: https://geminicli.com/docs/reference/tools/ "Gemini CLI tools reference"
[8]: https://opencode.ai/docs/agents/ "OpenCode agents"


| Tauri 壳层 | Tauri 用 Rust 后端和由系统 WebView 渲染的 HTML 前端构建桌面应用，提供 JS/Rust API 与 IPC，并依赖 WRY/TAO 集成系统窗口。 | Tauri 适合快速构建设置、工作区、命令面板等 UI，但 WebView/React 不应参与逐字符的终端状态更新；若采用它，应通过专用原生渲染 Surface 或严格的二进制帧协议隔离热路径。 | Tauri Docs [9] |
| WinUI 3 | WinUI 3 是微软推荐的新 Windows 桌面应用原生 UI 框架，基于 Windows App SDK，支持 C# 与 C++、高性能图形、高 DPI 和键鼠/触控输入。 | Windows-first 的生产架构可让 WinUI 3 承担所有非终端 Shell UI（Tabs、Pane chrome、Dashboard、Settings、Toast、Tray），并用 C++/Rust 终端 Surface 承担热路径。 | Microsoft Learn [10] |

[9]: https://v2.tauri.app/concept/architecture/ "Tauri Architecture"
[10]: https://learn.microsoft.com/en-us/windows/apps/winui/winui3/ "WinUI 3"


| Alacritty terminal core | Alacritty 是跨平台终端仿真器，仓库将终端逻辑分为独立的 `alacritty_terminal` crate，并在 Windows 支持运行；仓库提供 Apache-2.0 与 MIT 许可文件，README 表述整体为 Apache-2.0 发布。 | 作为 Rust 终端状态机/VT parser 的候选应在技术验证中优先评估；正式依赖前须锁定版本并以 Cargo 元数据/每文件头核对双许可选项。不要复制渲染层。 | Alacritty [11] |
| xterm.js | xterm.js 是面向浏览器的终端前端，具备 Attach、Search、Unicode grapheme、Web links、Clipboard、Image、Progress 等插件生态，仓库为 MIT。 | 可做 UX 原型和有限功能实现的备选；但它的终端屏幕与渲染运行于 WebView 主线程，不适合本项目正式版的独立高吞吐热路径。若采用，只能将输出批处理、限制 scrollback、启用 WebGL 并保留 Canvas fallback。 | xterm.js [12] |

[11]: https://github.com/alacritty/alacritty "Alacritty repository"
[12]: https://github.com/xtermjs/xterm.js/ "xterm.js repository"


| Warp 命令块 | Warp 以 Rust 和直接 GPU 渲染实现，强调复杂 UI 叠加、长输出、滚动与兼容现有 shell；其 block 依赖对当前 shell session 的深度集成。 | MVP 的 Command Block 必须是可选增强：先通过 shell integration 的明确 start/end 标记创建元数据，不侵入 TUI/全屏程序；无标记时回退为纯终端。 | Warp Engineering [13] |
| Warp 性能经验 | Warp 将 PTY 读取/ANSI 解析、渲染和滚动视作主要瓶颈，并将字形 atlas、批量绘制和低状态切换作为 GPU 渲染原则。 | 确定性能架构：有界 channel + VT 增量解析 + 单独渲染线程 + dirty-region render；UI 状态更新不得按字节/行驱动。 | Warp Engineering [13] |
| 粘贴保护 | Windows Terminal 默认对超过 5 KiB 的粘贴和多行粘贴给出警告；多行文本可能在 shell 中自动作为多条命令执行。 | AI Terminal 默认开启多行/大粘贴确认，并展示不可见控制字符；支持项目级例外但明确标记风险。 | Microsoft Learn [14] |
| 超链接策略 | Windows Terminal 可实验性地检测 URL 并需 Ctrl+点击后打开。 | OSC 8 /自动识别链接应显示真实 URI、仅以修饰键点击打开，并在 `file:`、自定义 scheme、Unicode 同形域名等高风险案例显示明确警告。 | Microsoft Learn [14] |

[13]: https://www.warp.dev/blog/how-warp-works "How Warp Works"
[14]: https://learn.microsoft.com/en-us/windows/terminal/customize-settings/interaction "Interaction settings in Windows Terminal"


| ConPTY 创建 | `CreatePseudoConsole` 以输入、输出管道和字符尺寸创建 PTY；当前输入/输出限制为同步 I/O；终端宿主负责呈现输出并把用户输入序列化到输入流。 | PTY Manager 需为每个会话创建专用同步管道，并用后台阻塞 I/O 线程桥接至有界异步队列；每次 Pane 字符格尺寸变更调用 ResizePseudoConsole，而非按像素变更。 | Microsoft Learn [15] |
| ConPTY 游标继承 | 若使用 `PSEUDOCONSOLE_INHERIT_CURSOR`，宿主需在后台异步响应该游标查询，否则可能导致请求挂起。 | MVP 不启用游标继承旗标；若未来启用，必须实现独立的 VT query/responder 与超时机制。 | Microsoft Learn [15] |
| ConPTY 关闭 | 关闭 HPCON 会向仍连接的客户端发送 `CTRL_CLOSE_EVENT`；旧版 Windows 可能等待，未关闭或排空输出管道会死锁。 | 会话关闭顺序：标记 Closing → 停止新输入 → 关闭输入 → 继续排空输出/设超时 → 后台调用 ClosePseudoConsole → 收集退出码 → 发布 Closed；不得从 reader thread 直接关闭。 | Microsoft Learn [16] |

[15]: https://learn.microsoft.com/en-us/windows/console/createpseudoconsole "CreatePseudoConsole"
[16]: https://learn.microsoft.com/en-us/windows/console/closepseudoconsole "ClosePseudoConsole"


| Kitty | Kitty 采用 GPU 与 SIMD、线程化渲染，强调低延迟，并提供 Shell Integration、会话、终端扩展协议和可组合的 kittens 工具。 | 借鉴“核心终端保持兼容、增强能力以可选协议/工具加入”的原则；AI Terminal 的 Agent、Git、项目增强不应污染 VT 基线或使 SSH/TUI 退化。 | Kitty Docs [17] |
| Tabby | Tabby 面向 Windows/macOS/Linux，提供 SSH/serial、PowerShell/WSL/CMD 等 Profile、分屏、可恢复 Tab、主题、快捷键和插件模块；仓库采用 MIT。 | 可借鉴 Profile/插件/连接管理的信息架构；但其 Electron 架构和包体/内存开销不符合本项目 Windows-first 的性能优先目标。 | Tabby [18] |

[17]: https://sw.kovidgoyal.net/kitty/ "kitty"
[18]: https://github.com/eugeny/tabby "Tabby repository"


| iTerm2 Shell Integration | iTerm2 的 shell integration 可自动或手动装入，跟踪命令历史、cwd、主机名与退出码；无法安装时可用正则触发器推断 prompt、主机和目录。 | AI Terminal 的 command/cwd 采集应按可信度分级：首选显式 OSC/shell hook，其次可配置 prompt 解析，最后仅显示“未知”；对 remote/production 默认不自动改写 dotfiles。 | iTerm2 Docs [19] |
| Rio | Rio 是开源的现代跨平台终端，提供 Windows 10+ MSI 与便携发布，属于 Rust/GPU 路线的较轻量参考。 | 可观察其 Windows 打包与 GPU 技术栈，但在本项目的 MVP 中不直接复用其整机代码；需优先选择具备稳定嵌入接口与成熟 Windows 自动化测试的核心。 | Rio [20] |

[19]: https://iterm2.com/documentation-shell-integration.html "iTerm2 Shell Integration"
[20]: https://rioterm.com/ "Rio Terminal"


| Hyper | Hyper 明确为 Electron 终端，使用 Node.js 扩展、React/Redux 组件装饰和 xterm.js 终端底座。 | 其成熟的主题/扩展体验值得借鉴，但也说明“插件直接触达渲染树”的安全与性能风险；AI Terminal 插件必须经版本化、权限化的 Host API，而非任意 UI/进程访问。 | Hyper [21] |
| Wave 工作区 | Wave 通过 Workspace Switcher 管理保存的工作区，自动持久化 Tab、布局、终端和 AI 历史，并区分临时工作区与保存工作区。 | 采纳“显式保存 vs 临时工作区”模型，但 AI Terminal 默认仅保存元数据；终端全文/AI 历史应采用 Off、Metadata Only、Full 三档并在 Full 下提示敏感信息风险。 | Wave Docs [22] |

[21]: https://hyper.is/ "Hyper"
[22]: https://docs.waveterm.dev/workspaces "Wave Terminal Workspaces"


| Aider | Aider 围绕终端结对编程，提供 Git 集成、仓库地图、脚本化、通知和多 LLM 配置。 | Agent Dashboard 必须将 Git 上下文作为“只读辅助元数据”展示：分支、工作树变更、近期命令/测试结果；不能篡改或替代 Aider 的提交与确认行为。 | Aider Docs [23] |
| GitHub Copilot CLI | Copilot CLI 运行在终端中，并可委派 Explore、Task、Review、Research 等子 Agent；支持用户/仓库/组织层级的自定义 Agent Profile。 | 统一 Agent 数据模型需要有 `parentSessionId`、`subagentCount`、`agentProfile` 与结构化能力，状态页需聚合而不是把子 Agent 全部等同为独立顶层 Tab。 | GitHub Docs [24] |

[23]: https://aider.chat/docs/ "Aider Documentation"
[24]: https://docs.github.com/en/copilot/how-tos/copilot-cli/use-copilot-cli/overview "Using GitHub Copilot CLI"

