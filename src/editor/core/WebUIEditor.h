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

/**
 * @brief WebUI 编辑器系统
 * 
 * 负责启动 Web 服务器，提供浏览器访问的全功能编辑器。
 */
class EDITOR_API WebUIEditor {
public:
    WebUIEditor();
    ~WebUIEditor();

    /**
     * @brief 启动 WebUI 服务器
     * @param port 监听端口
     * @return 是否启动成功
     */
    bool Start(int port = 8080);

    /**
     * @brief 停止 WebUI 服务器
     */
    void Stop();

    /**
     * @brief 更新 WebUI（处理连接、同步状态等）
     */
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
