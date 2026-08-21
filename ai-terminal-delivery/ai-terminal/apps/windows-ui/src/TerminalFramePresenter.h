#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace AITerminal::Rendering {

struct GridSize {
    uint32_t columns{};
    uint32_t rows{};
};

struct CellRun {
    uint32_t row{};
    uint32_t column{};
    std::wstring text;
    uint32_t foregroundArgb{ 0xFFE8EDF7 };
    uint32_t backgroundArgb{ 0xFF0E1117 };
    uint32_t styleFlags{};
};

struct FrameDelta {
    uint64_t sequence{};
    GridSize grid;
    std::vector<CellRun> cells;
    uint32_t cursorRow{};
    uint32_t cursorColumn{};
    bool cursorVisible{ true };
    bool fullRepaint{};
};

/// Receives only viewport-level terminal deltas from the service.
/// DirectWrite/D3D resources are private to the UI process and must never be used from a PTY reader.
class TerminalFramePresenter final {
public:
    TerminalFramePresenter() = default;

    void SetDpi(float dpi) noexcept;
    void Apply(const FrameDelta& delta);
    void InvalidateAll() noexcept;
    [[nodiscard]] GridSize CurrentGrid() const noexcept;
    [[nodiscard]] uint64_t LastSequence() const noexcept;

private:
    void EnsureGrid(GridSize grid);
    void DrawDirtyCell(const CellRun& cell);
    void DrawCursor();

    GridSize m_grid{};
    uint64_t m_sequence{};
    float m_dpi{ 96.0F };
    bool m_needsFullRepaint{ true };
};

} // namespace AITerminal::Rendering
