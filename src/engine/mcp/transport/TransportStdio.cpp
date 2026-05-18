#include "TransportStdio.h"
#include <iostream>
#include <sstream>
#include <cstdio>
#ifdef _WIN32
#include <windows.h>
#endif

namespace Prisma {
namespace MCP {

TransportStdio::TransportStdio() = default;
TransportStdio::~TransportStdio() { Stop(); }

bool TransportStdio::Start(MCPMessageHandler handler) {
    if (m_Running) return false;
    m_Handler = std::move(handler);
    m_Running = true;
    m_ReadThread = std::thread(&TransportStdio::readLoop, this);
    return true;
}

void TransportStdio::Stop() {
    if (!m_Running) return;
    m_Running = false;
    
    if (m_ReadThread.joinable()) {
#ifdef _WIN32
        // 取消待决的控制台输入读取，以唤醒因 std::getline(std::cin) 阻塞的读取线程
        HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
        if (hStdin != INVALID_HANDLE_VALUE) {
            CancelIoEx(hStdin, nullptr);
        }
        m_ReadThread.join();
#else
        // 在 Linux/POSIX 上，没有完美的办法取消阻塞在 std::cin 的 read/getline。
        // 如果我们正在退出且该线程仍然阻塞，直接销毁进程。
        // 这虽然粗暴，但在 CLI/MCP 模式下是必要的，否则会一直挂起。
        std::terminate(); 
#endif
    }
}

bool TransportStdio::Send(const glz::json_t& message) {
    if (!m_Running) return false;
    try {
        std::string buffer;
        auto wEc = glz::write_json(message, buffer);
        if (wEc) return false;
        std::string output = buffer + "\n";
        std::cout << output << std::flush;
        return true;
    } catch (...) {
        return false;
    }
}

bool TransportStdio::IsConnected() const {
    return m_Running;
}

void TransportStdio::readLoop() {
    std::string line;
    while (m_Running && std::getline(std::cin, line)) {
        if (!m_Running) break;
        processLine(line);
    }
    m_Running = false;
}

void TransportStdio::processLine(const std::string& line) {
    if (line.empty()) return;
    try {
        glz::json_t json;
        auto ec = glz::read_json(json, line);
        if (ec) throw std::runtime_error("Parse error");

        if (m_Handler) {
            m_Handler(json);
        }
    } catch (const std::exception& e) {
        glz::json_t err = glz::json_t::object_t{
            {"jsonrpc", "2.0"},
            {"id", {}},
            {"error", glz::json_t::object_t{{"code", -32700.0}, {"message", std::string("Parse error: ") + e.what()}}}
        };
        Send(err);
    }
}

} // namespace MCP
} // namespace Prisma
