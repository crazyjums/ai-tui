#include "TerminalFramePresenter.h"

#include <algorithm>

namespace AITerminal::Rendering {

void TerminalFramePresenter::SetDpi(float dpi) noexcept {
    // A real implementation recreates DirectWrite text formats and glyph-atlas metrics here.
    m_dpi = std::max(72.0F, dpi);
    m_needsFullRepaint = true;
}

void TerminalFramePresenter::Apply(const FrameDelta& delta) {
    // Sequence gaps indicate that a slow consumer dropped intermediate visual frames. That is
    // acceptable only when the service marks the next payload as a full repaint.
    if (m_sequence != 0 && delta.sequence > m_sequence + 1 && !delta.fullRepaint) {
        m_needsFullRepaint = true;
    }

    EnsureGrid(delta.grid);
    m_sequence = delta.sequence;
    m_needsFullRepaint = m_needsFullRepaint || delta.fullRepaint;

    // Important: `cells` is already limited to this viewport's dirty runs. Never recreate XAML
    // controls or rerender the full scrollback for an ordinary VT write.
    for (const auto& cell : delta.cells) {
        DrawDirtyCell(cell);
    }
    DrawCursor();
    m_needsFullRepaint = false;
}

void TerminalFramePresenter::InvalidateAll() noexcept {
    m_needsFullRepaint = true;
}

GridSize TerminalFramePresenter::CurrentGrid() const noexcept {
    return m_grid;
}

uint64_t TerminalFramePresenter::LastSequence() const noexcept {
    return m_sequence;
}

void TerminalFramePresenter::EnsureGrid(GridSize grid) {
    if (grid.columns != m_grid.columns || grid.rows != m_grid.rows) {
        m_grid = grid;
        m_needsFullRepaint = true;
        // Real implementation allocates/reuses D3D render targets and asks the service for a
        // full viewport frame after the ConPTY resize acknowledgement.
    }
}

void TerminalFramePresenter::DrawDirtyCell(const CellRun&) {
    // Phase 0 implementation point:
    // 1. Resolve DirectWrite font fallback and grapheme shaping.
    // 2. Lookup/rasterize glyph in the Direct2D/D3D glyph atlas.
    // 3. Draw background, decorations and glyph quad into the dirty rect.
    // This intentionally stays free of WinUI controls and terminal-parser state.
}

void TerminalFramePresenter::DrawCursor() {
    // Render block/bar/underline cursor only after the screen delta has committed.
}

} // namespace AITerminal::Rendering
