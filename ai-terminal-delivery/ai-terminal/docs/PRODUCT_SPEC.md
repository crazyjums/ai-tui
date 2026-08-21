# AI Terminal Product Specification

**版本：** 0.1  
**产品名称：** AI Terminal（工作名）  
**产品定位：** 面向 Windows 开发者的下一代 AI Coding Native Terminal。它提供生产级 Windows terminal emulator 的可靠性，同时把 Workspace、并行 Agent、权限等待、项目和开发任务状态放到更高效的工作流中。

## 1. 产品愿景

AI Terminal 的目标是让开发者打开一个应用后，能够在不丢失传统命令行自由度的前提下，管理本地 shell、WSL、SSH、项目环境以及多个 AI Coding Agent。它从 Windows Terminal 的可靠会话模型、Warp 的现代交互、VS Code 的 Workspace 思路、tmux 的 Pane 能力和 Raycast 的键盘效率中取长，但不会重做编辑器、LSP 或 Debugger。

> **一句话价值主张：** 当 Claude、Codex、Gemini 或其他 Agent 在不同项目中执行、等待批准或完成任务时，开发者无需来回切换窗口或盯住滚屏，也能保持对每个终端会话的掌控。

## 2. 目标用户与核心痛点

| 用户画像 | 当前工作方式 | 关键痛点 | AI Terminal 解决方式 |
| --- | --- | --- | --- |
| 全栈开发者 | PowerShell + WSL + VS Code + 两到四个 AI CLI。 | Agent 混在普通 Tab 中，难判断哪个等待、哪个失败、哪个项目有改动。 | Workspace、Agent state、Git 摘要和 status-first Tab。 |
| 后端/平台工程师 | 多个本地服务、Docker/K8s、SSH 跳板机和测试命令并发。 | 多 Pane 很难长期整理；重开工具后需要手动恢复环境。 | 保存的 project-aware Workspace、可恢复布局、SSH/Profile 管理。 |
| AI 重度使用者 | Claude/Codex/Gemini/OpenCode 并行处理 feature、tests、docs。 | 权限请求容易被埋在滚屏中；无法可靠地追踪多 Agent 的状态。 | 统一 Dashboard、Waiting/Failed 通知、Adapter 证据模型。 |
| Windows 原生开发者 | PowerShell、CMD、Developer PowerShell、Git Bash、WSL 混用。 | 通用 Web 工具对 IME、Credential Manager、Tray、Windows 通知和 Profile 缺乏原生感。 | WinUI 3 Shell、ConPTY、Toast/Taskbar/Tray 与 Windows Credential Manager。 |

## 3. 使用场景与关键旅程

### 场景 A：一个项目、两个并行 Agent

开发者打开 `AI-CICD` Workspace，系统恢复三列环境：左侧 Claude Code 执行后端任务，中间 Codex 处理前端，右侧 `npm run dev` 和测试。Claude 进入 `WaitingPermission` 时，Tab、任务栏和 Toast 仅提示“Claude — backend needs attention”；开发者点击后跳回原始终端 prompt 完成批准。此时 Codex、dev server 和 tests 不受影响。

### 场景 B：本地开发与远程事故排查

开发者从 Quick Launch 启动 `SSH prod` 并同时打开 WSL Ubuntu 测试环境。SSH 连接的 key 和 host 信息由系统凭据引用，所有远程会话保留普通终端兼容性。Workspace 提示当前项目的 Git branch 与本地 Agent，但不把远程服务器变成一个隐式受信任项目。

### 场景 C：第二天恢复工作

开发者关闭窗口或应用崩溃后再次打开 AI Terminal。产品询问是否恢复 `AI-CICD` Workspace 的布局、cwd、Profile 和启动命令。未选择全文保存时，不展示或上传历史终端输出；如果底层 session 已消失，UI 明确标记为“Session unavailable — restart available”，而不是伪造恢复。

## 4. 功能地图

| 域 | MVP | 后续阶段 |
| --- | --- | --- |
| Terminal | ConPTY、PowerShell/CMD/WSL、真实 VT、字体/主题、搜索、鼠标/resize/clipboard。 | Image protocol、advanced graphics、serial、record/replay。 |
| Layout | Multi-tab、任意嵌套 Pane、resize、focus、zoom、duplicate、restore。 | Read-only mirrored Pane、跨窗口拖动、terminal mux attach。 |
| Workspace | 保存/恢复布局、cwd、profile、project、启动命令和 metadata。 | Team workspace template、remote workspace、云端只读查看。 |
| AI Agent | Claude/Codex 首批 adapter、Generic fallback、统一状态、Dashboard、waiting/complete/failed 通知。 | Gemini/OpenCode/Aider/Copilot 深度 adapter、sub-agent tree、Agent Session Protocol。 |
| Project/Git | 保存项目、快捷打开 shell/Agent/VS Code/Explorer、只读 Git 状态栏。 | Git Panel、diff/commit 辅助、worktree 管理。 |
| Remote | SSH Manager、SSH config、key reference、WSL discovery。 | Persisted remote server / multiplex domain、mobile/web viewer。 |
| Settings | 完整 GUI、JSON advanced config、可重绑快捷键、Profiles、Privacy。 | Settings sync、组织策略、policy management。 |
| Extensibility | 内置 adapter interfaces 与 manifest。 | 签名插件、WASM Plugin Host、MCP client/server。 |

## 5. MVP 范围与验收边界

MVP 的定义是：**十个可持续使用的功能优先于五十个演示性功能。** 因此，真正需要打磨的是 terminal emulator、tabs/panes、Workspace、PowerShell/CMD/WSL、Claude/Codex 快速启动、Theme、Settings 与 Command Palette。SSH、Git 和 Agent Dashboard 可以以只读/轻量实现切入，但不得假装为完整 Git client 或完整远程 IDE。

| 必须在 MVP 验收 | 关键验收标准 |
| --- | --- |
| PowerShell / CMD / WSL | 可在各自 session 正常运行 `Get-ChildItem`、`dir`、`ls -la`，调整 Pane 后继续正常交互。 |
| 交互程序 | `vim`、`python`、`ssh` 与 full-screen TUI 不被 Command Block/Agent 观察破坏。 |
| 多 Tab / Pane | PowerShell、Claude、Codex、WSL 同时存在且互不阻塞；可 split、focus、resize、close、restore。 |
| Workspace | 保存并恢复项目、cwd、Profile、布局与无敏感 metadata；未知 Workspace 必须触发 trust review。 |
| Agent 可见性 | 用户可从 Claude/Codex 启动器创建 session，识别运行/退出与高置信的等待状态；通知仅在需求用户注意时触发。 |
| 隐私安全 | 默认 MetadataOnly；不自动批准命令；多行粘贴警告默认开启；SSH secret 不落明文。 |
| 性能 | 高输出下 UI 不明显冻结，输入、Tab 切换、scroll 保持可用；各 Session 故障隔离。 |

| 绝不承诺进入 MVP | 原因 |
| --- | --- |
| 完整代码编辑器、文件树、LSP/Debugger。 | 会使 Terminal First 定位漂移。 |
| 自行推断并批准所有 Agent 权限。 | 缺乏安全可验证性，违背原 CLI 权限模型。 |
| 强制 Command Block 覆盖 TUI/SSH。 | 会破坏传统 Terminal 兼容性。 |
| 通用第三方插件任意执行。 | 需先完成权限、签名、隔离和 API 稳定性。 |
| 无条件恢复已消失的 ConPTY process。 | PTY 依附关系决定了传统进程不一定可重新 attach。 |

## 6. 成功指标

产品成功不是按下载量或功能数量衡量，而是用开发者实际控制多任务开发环境的效率判断。内部 alpha 需要以可测信号验证：启动可接受、命令输入不被输出抢占、用户能在一个列表中找到所有等待操作、环境恢复不要求重复手工配置、且用户不会因安全/隐私不确定性而关闭 Agent integration。

| 指标类别 | MVP 目标 | 测量方式 |
| --- | --- | --- |
| 交互延迟 | 高输出时输入保持即时感。 | keystroke-to-render、frame pacing、queue saturation benchmark。 |
| 任务可见性 | 等待批准/失败能在一次操作内定位。 | Dashboard focus flow E2E、Toast → Pane 跳转成功率。 |
| 恢复效率 | 保存 Workspace 后恢复基本环境无需重输 cwd/profile。 | Recovery E2E，统计恢复会话/布局正确率。 |
| 可靠性 | 一个 session/service plugin 问题不带崩窗口和其他 session。 | 故障注入、process kill、reader backpressure。 |
| 隐私 | 默认不会在可导出的文件中留下 terminal body 或 secret。 | storage audit、redaction tests、config snapshot tests。 |

## 7. 风险、约束与产品决策

| 风险 | 用户影响 | 产品决策 |
| --- | --- | --- |
| AI CLI 无稳定状态协议 | 错误状态会误导用户。 | 显示证据来源/置信度；无可靠信号时不分类。 |
| Native renderer 工程成本 | MVP 可能延迟。 | Phase 0 先冻结 ConPTY → parser → renderer 验证链路，UI 原型与生产 renderer 分轨。 |
| 用户把 AI Terminal 当 IDE | 范围膨胀、性能下降。 | Project/Workspace 仅服务于启动和状态，不做编辑器。 |
| 远程/SSH 敏感性 | 凭据与命令泄露。 | Credential Manager、最小历史、host trust、默认无自动 shell injection。 |
| 过度通知 | 用户关闭所有提醒。 | 只在 Waiting/Failed/Completed（可配）提醒；Running 仅计数。 |

## 8. 发布路径

| 阶段 | 用户价值 | 发布门槛 |
| --- | --- | --- |
| Phase 0 — Private Technical Preview | 验证 PowerShell/CMD/WSL、渲染和 ConPTY 行为。 | VT/TUI 核心回归通过，无 UI 卡死。 |
| Phase 1 — MVP Alpha | Tab、Pane、Workspace、Theme、Palette 与 GUI Settings。 | 高输出/恢复/快捷键 E2E 通过。 |
| Phase 2 — Developer Beta | Project、SSH、Git read-only、Session metadata、Tray/Toast。 | SSH secret/host trust 审计与 Windows notifications 通过。 |
| Phase 3 — AI Coding Beta | Claude/Codex、Dashboard、Waiting semantics、Agent notification。 | Adapter evidence、privacy、no-auto-approve 测试通过。 |
| Phase 4 — Public Preview | 签名插件、远程扩展、MCP 可选能力。 | 外部安全审计、plugin sandbox、兼容性矩阵达标。 |

## 9. 产品原则清单

1. **终端优先于面板。** 任何时候用户都能像使用可信任 Terminal 一样使用它。
2. **状态优先于花哨动画。** 运行、等待、失败、完成必须稳定、可解释、可操作。
3. **快捷键优先于鼠标。** 高频操作应不要求离开键盘。
4. **恢复优先于重建。** Workspace 必须节省重新开 shell、cd、启动 Agent 的时间。
5. **真实信息优先于猜测。** Agent 状态必须保留来源与置信度。
6. **隐私默认而非事后选项。** 不默认保存终端全文、命令或凭据。
7. **增强不破坏。** Shell、SSH、TUI 与传统 CLI 兼容性高于 Command Block 和 Dashboard 美观。
