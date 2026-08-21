# Open Source Reuse and License Register

This register records both sources investigated for AI Terminal and the stricter decision of whether code is currently reused. **No third-party terminal implementation has been copied into this scaffold.** Future distribution must regenerate this file from locked dependency metadata, retain upstream notices, and include an SBOM.

| Project | Repository | License status | Proposed use | Current modification/reuse status | Required action before shipping |
| --- | --- | --- | --- | --- | --- |
| Alacritty / `alacritty_terminal` | https://github.com/alacritty/alacritty | Repository publishes Apache-2.0 and MIT license files; validate exact crate metadata at lock time. | Candidate VT parser/screen model for Phase 0 evaluation. | **Not included.** | Pin version, inspect Cargo metadata/transitive licenses, preserve notices, validate VT behavior. |
| Windows Terminal | https://github.com/microsoft/terminal | MIT. | Behavior/test/architecture reference. | **Not included.** | If any source is copied, retain per-file attribution and MIT notice. |
| WezTerm | https://github.com/wezterm/wezterm | Verify at pinned revision. | Domain/session model reference. | **Not included.** | Do not import broad application modules solely for multiplexing. |
| Ghostty | https://github.com/ghostty-org/ghostty | Verify at pinned revision; standalone core API is not yet stable. | Core/UI separation reference. | **Not included.** | Do not depend on unstable public API without vendor/legal review. |
| xterm.js | https://github.com/xtermjs/xterm.js | MIT. | Optional browser prototype/reference only. | **Not included.** | Preserve notices if added to an internal prototype; never make it the production renderer by accident. |
| Tabby | https://github.com/Eugeny/tabby | MIT. | Profile/plugin UX reference. | **Not included.** | Audit copied modules individually if any are considered. |
| Tauri | https://github.com/tauri-apps/tauri | MIT OR Apache-2.0 (verify pinned version). | Possible internal developer tool shell, not current product runtime. | **Not included.** | Keep WebView prototype clearly separate from native production UI. |

## First-party scaffold license

Unless individual files state otherwise, source written for this project is intended for **MIT OR Apache-2.0** dual licensing. This must be finalized with top-level `LICENSE-MIT`, `LICENSE-APACHE`, `NOTICE`, dependency lock files and SBOM before a public release.
