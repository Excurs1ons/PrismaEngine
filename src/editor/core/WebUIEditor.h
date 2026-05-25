#pragma once

#include "Export.h"
#include <string>
#include <memory>
#include <thread>
#include <atomic>

namespace httplib {
    class Server;
}

namespace Prisma {

/* WebUI 编辑器系统 */
class EDITOR_API WebUIEditor {
public:
    WebUIEditor();
    ~WebUIEditor();

    /**
     * 启动 WebUI 服务器
     */
    bool Start(int port = 8080);

    // 停止 WebUI 服务器
    void Stop();

    // 更新 WebUI（处理连接、同步状态等）
    void Update();

    bool IsRunning() const { return m_running; }

private:
    std::string ProcessMCPRequest(const std::string& requestBody);

private:
    int m_port = 8080;
    std::atomic<bool> m_running{false};
    std::thread m_serverThread;
    std::unique_ptr<httplib::Server> m_server;
};

} // namespace Prisma
