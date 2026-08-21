#include "ServiceConnection.h"

#include <mutex>
#include <utility>

namespace AITerminal::Transport {

struct ServiceConnection::Impl {
    std::mutex mutex;
    FrameCallback onFrame;
    StateCallback onState;
    DisconnectCallback onDisconnect;
    bool connected{};

    // Phase 0 implementation owns HANDLE to \\.\pipe\AITerminal\v1 and a worker thread.
    // The worker performs length-prefixed protobuf reads and dispatches them to the WinUI
    // DispatcherQueue. Every envelope is schema/version checked before callbacks are invoked.
};

ServiceConnection::ServiceConnection() : m_impl(std::make_unique<Impl>()) {}
ServiceConnection::~ServiceConnection() { Disconnect(); }

bool ServiceConnection::Connect() {
    std::scoped_lock lock{ m_impl->mutex };
    // CreateFileW with current-user ACL validation and protocol handshake belongs here.
    // Failure must leave the UI usable and surface a reconnect banner rather than terminate it.
    m_impl->connected = true;
    return true;
}

void ServiceConnection::Disconnect() noexcept {
    std::scoped_lock lock{ m_impl->mutex };
    if (!m_impl->connected) { return; }
    m_impl->connected = false;
    // Cancel synchronous/overlapped pipe work, join worker, and close the handle.
}

bool ServiceConnection::SendInput(std::wstring_view, std::span<const std::byte>) {
    std::scoped_lock lock{ m_impl->mutex };
    // Serialize a SendInput protobuf without passing user text through shell interpolation.
    return m_impl->connected;
}

bool ServiceConnection::Resize(std::wstring_view, Rendering::GridSize) {
    std::scoped_lock lock{ m_impl->mutex };
    // UI coalesces pointer drag events; service performs ResizePseudoConsole after its input
    // queue reaches a safe boundary.
    return m_impl->connected;
}

void ServiceConnection::SubscribeViewport(std::wstring_view, uint64_t, uint32_t) {
    std::scoped_lock lock{ m_impl->mutex };
    // The service only sends dirty data for this viewport; full scrollback is never mirrored
    // into XAML state.
}

void ServiceConnection::OnFrame(FrameCallback callback) {
    std::scoped_lock lock{ m_impl->mutex };
    m_impl->onFrame = std::move(callback);
}

void ServiceConnection::OnAgentState(StateCallback callback) {
    std::scoped_lock lock{ m_impl->mutex };
    m_impl->onState = std::move(callback);
}

void ServiceConnection::OnDisconnect(DisconnectCallback callback) {
    std::scoped_lock lock{ m_impl->mutex };
    m_impl->onDisconnect = std::move(callback);
}

} // namespace AITerminal::Transport
