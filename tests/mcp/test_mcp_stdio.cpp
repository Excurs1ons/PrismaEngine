// MCP Stdio Integration Test Server
// Runs a real MCPServer with StdioTransport.
// Accepts JSON-RPC messages via stdin, responds via stdout.
// Test with: python tests/mcp/test_mcp_client.py

#define ENGINE_API

#include <iostream>
#include <thread>
#include <chrono>
#include <nlohmann/json.hpp>
#include "mcp/MCPServer.h"
#include "mcp/transport/TransportStdio.h"
#include "mcp/MCPTool.h"
#include "mcp/MCPSession.h"

using namespace Prisma::MCP;

class EchoTool : public MCPTool {
public:
    std::string_view GetName() const override { return "echo"; }
    std::string_view GetDescription() const override { return "Echo back the input message."; }
    std::string_view GetCategory() const override { return "test"; }
    nlohmann::json GetInputSchema() const override {
        return {{"type", "object"}, {"properties", {{"message", {{"type", "string"}}}}}};
    }
    nlohmann::json Execute(const nlohmann::json& args) override {
        return {{"echo", args.value("message", "")}};
    }
};

class AddTool : public MCPTool {
public:
    std::string_view GetName() const override { return "add"; }
    std::string_view GetDescription() const override { return "Add two integers."; }
    std::string_view GetCategory() const override { return "test"; }
    nlohmann::json GetInputSchema() const override {
        return {
            {"type", "object"},
            {"properties", {
                {"a", {{"type", "integer"}}},
                {"b", {{"type", "integer"}}}
            }},
            {"required", {"a", "b"}}
        };
    }
    nlohmann::json Execute(const nlohmann::json& args) override {
        int a = args.value("a", 0);
        int b = args.value("b", 0);
        return {{"result", a + b}};
    }
};

int main() {
    ToolRegistry registry;
    registry.AddTool(std::make_unique<EchoTool>());
    registry.AddTool(std::make_unique<AddTool>());

    auto transport = std::make_unique<TransportStdio>();
    MCPServer server(std::move(transport));
    auto session = std::make_shared<MCPSession>();
    server.SetSession(session);

    if (!server.Start(&registry)) {
        return 1;
    }

    // Run until stdin closes
    while (server.IsRunning()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    return 0;
}
