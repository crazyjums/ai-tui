#pragma once

#include "TerminalFramePresenter.h"

#include <functional>
#include <memory>
#include <span>
#include <string_view>

namespace AITerminal::Transport {

/// UI-side owner of the user-ACL-protected named-pipe connection. The production implementation
/// runs pipe reads on a dedicated worker and posts strongly typed frame/state callbacks to the
/// WinUI dispatcher. It never invokes user callbacks while holding transport locks.
class ServiceConnection final {
public:
    using FrameCallback = std::function<void(const Rendering::FrameDelta&)>;
    using StateCallback = std::function<void(std::wstring_view sessionId, std::wstring_view state, std::wstring_view summary, uint32_t confidence)>;
    using DisconnectCallback = std::function<void(std::wstring_view diagnostic)>;

    ServiceConnection();
    ~ServiceConnection();

    ServiceConnection(const ServiceConnection&) = delete;
    ServiceConnection& operator=(const ServiceConnection&) = delete;

    bool Connect();
    void Disconnect() noexcept;
    bool SendInput(std::wstring_view sessionId, std::span<const std::byte> utf8VtInput);
    bool Resize(std::wstring_view sessionId, Rendering::GridSize grid);
    void SubscribeViewport(std::wstring_view sessionId, uint64_t startLine, uint32_t visibleRows);

    void OnFrame(FrameCallback callback);
    void OnAgentState(StateCallback callback);
    void OnDisconnect(DisconnectCallback callback);

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace AITerminal::Transport
