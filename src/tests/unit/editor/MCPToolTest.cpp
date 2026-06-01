#include <gtest/gtest.h>
#include <glaze/glaze.hpp>
#include <glaze/json/generic.hpp>
#include <string>
#include <optional>

// 纯逻辑测试：验证 MCP JSON-RPC 2.0 请求/响应格式
// 使用 glaze 的 json_t 类型构造 JSON 字符串，验证字段存在性和正确性。
// 不涉及实际 MCP 工具调用。

namespace Prisma {
namespace MCP {

// 与引擎 MCPJson.h 中的 MCPRequest 一致
struct MCPRequest {
    std::string jsonrpc = "2.0";
    std::string method;
    glz::json_t params = glz::json_t::object_t{};
    glz::json_t id = {};
};

// 与引擎 MCPJson.h 中的 MCPResponse 一致
struct MCPResponse {
    std::string jsonrpc = "2.0";
    glz::json_t id = {};
    glz::json_t result = glz::json_t::object_t{};
    std::optional<glz::json_t> error = std::nullopt;

    static MCPResponse Success(glz::json_t id, glz::json_t result) {
        MCPResponse r;
        r.id = std::move(id);
        r.result = std::move(result);
        return r;
    }

    static MCPResponse Error(glz::json_t id, int code,
                             const std::string& message,
                             glz::json_t data = {}) {
        MCPResponse r;
        r.id = std::move(id);
        r.error = glz::json_t::object_t{
            {"code", static_cast<double>(code)},
            {"message", message}
        };
        if (!data.is_null()) {
            (*r.error).get_object()["data"] = std::move(data);
        }
        return r;
    }
};

namespace ErrorCode {
    constexpr int ParseError     = -32700;
    constexpr int InvalidRequest = -32600;
    constexpr int MethodNotFound = -32601;
    constexpr int InvalidParams  = -32602;
    constexpr int InternalError  = -32603;
}

} // namespace MCP
} // namespace Prisma

namespace {
using namespace Prisma::MCP;

// ========== JSON-RPC 请求格式测试 ==========

TEST(MCPToolTest, RequestHasRequiredFields) {
    MCPRequest req;
    req.method = "test_method";
    req.id = 1;
    req.params = glz::json_t::object_t{{"key", "value"}};

    // 验证字段存在
    EXPECT_EQ(req.jsonrpc, "2.0");
    EXPECT_EQ(req.method, "test_method");
    EXPECT_TRUE(req.id.is_number());
    EXPECT_TRUE(req.params.is_object());
}

TEST(MCPToolTest, RequestSerializesCorrectly) {
    MCPRequest req;
    req.method = "execute_command";
    req.id = 42;
    req.params = glz::json_t::object_t{
        {"command", "test"},
        {"args", glz::json_t::array_t{1, 2, 3}}
    };

    // 序列化为 JSON 字符串并验证
    std::string json = glz::write_json(req).value_or("");

    // 验证 JSON 包含必需字段
    EXPECT_NE(json.find("\"jsonrpc\""), std::string::npos);
    EXPECT_NE(json.find("\"method\""), std::string::npos);
    EXPECT_NE(json.find("\"params\""), std::string::npos);
    EXPECT_NE(json.find("\"id\""), std::string::npos);
    EXPECT_NE(json.find("\"2.0\""), std::string::npos);
    EXPECT_NE(json.find("\"execute_command\""), std::string::npos);
    EXPECT_NE(json.find("42"), std::string::npos);
}

TEST(MCPToolTest, RequestDefaultJsonrpcVersion) {
    MCPRequest req;
    EXPECT_EQ(req.jsonrpc, "2.0");
}

TEST(MCPToolTest, RequestDefaultParamsIsEmptyObject) {
    MCPRequest req;
    EXPECT_TRUE(req.params.is_object());
    EXPECT_TRUE(req.params.get_object().empty());
}

// ========== JSON-RPC 响应格式测试 ==========

TEST(MCPToolTest, SuccessResponseHasRequiredFields) {
    glz::json_t id = 1;
    glz::json_t result = glz::json_t::object_t{{"status", "ok"}};
    auto resp = MCPResponse::Success(std::move(id), std::move(result));

    // 验证成功响应没有 error
    EXPECT_FALSE(resp.error.has_value());
    EXPECT_EQ(resp.jsonrpc, "2.0");
    EXPECT_TRUE(resp.id.is_number());
    EXPECT_TRUE(resp.result.is_object());

    // 验证 result 包含正确内容
    auto& obj = resp.result.get_object();
    EXPECT_TRUE(obj.contains("status"));
    EXPECT_EQ(obj.at("status").get_string(), "ok");
}

TEST(MCPToolTest, SuccessResponseSerializesCorrectly) {
    auto resp = MCPResponse::Success(glz::json_t(100),
        glz::json_t::object_t{{"message", "done"}});

    std::string json = glz::write_json(resp).value_or("");

    EXPECT_NE(json.find("\"jsonrpc\""), std::string::npos);
    EXPECT_NE(json.find("\"result\""), std::string::npos);
    EXPECT_NE(json.find("\"id\""), std::string::npos);
    EXPECT_NE(json.find("100"), std::string::npos);
    EXPECT_NE(json.find("\"done\""), std::string::npos);
    // 成功响应不应包含 error 字段
    EXPECT_EQ(json.find("\"error\""), std::string::npos);
}

TEST(MCPToolTest, ErrorResponseHasRequiredFields) {
    auto resp = MCPResponse::Error(nullptr, -32601, "Method not found");

    EXPECT_TRUE(resp.error.has_value());
    EXPECT_EQ(resp.jsonrpc, "2.0");
    EXPECT_TRUE(resp.id.is_null());

    // 验证 error 格式
    auto& errObj = resp.error->get_object();
    EXPECT_TRUE(errObj.contains("code"));
    EXPECT_TRUE(errObj.contains("message"));
    EXPECT_EQ(static_cast<int>(errObj.at("code").get_number()), -32601);
    EXPECT_EQ(errObj.at("message").get_string(), "Method not found");

    // 错误响应不应包含 result
    // （我们的 struct 在错误时有 result 默认为空对象，但序列化时不应包含）
}

TEST(MCPToolTest, ErrorResponseSerializesCorrectly) {
    auto resp = MCPResponse::Error(glz::json_t(1),
        -32700, "Parse error",
        glz::json_t::object_t{{"details", "invalid JSON"}});

    std::string json = glz::write_json(resp).value_or("");

    EXPECT_NE(json.find("\"jsonrpc\""), std::string::npos);
    EXPECT_NE(json.find("\"error\""), std::string::npos);
    EXPECT_NE(json.find("\"code\""), std::string::npos);
    EXPECT_NE(json.find("\"message\""), std::string::npos);
    EXPECT_NE(json.find("-32700"), std::string::npos);
    EXPECT_NE(json.find("\"Parse error\""), std::string::npos);
    EXPECT_NE(json.find("\"details\""), std::string::npos);
}

// ========== JSON-RPC 错误码测试 ==========

TEST(MCPToolTest, ErrorCodeValues) {
    EXPECT_EQ(ErrorCode::ParseError, -32700);
    EXPECT_EQ(ErrorCode::InvalidRequest, -32600);
    EXPECT_EQ(ErrorCode::MethodNotFound, -32601);
    EXPECT_EQ(ErrorCode::InvalidParams, -32602);
    EXPECT_EQ(ErrorCode::InternalError, -32603);
}

TEST(MCPToolTest, ErrorResponseResultFieldNotPresent) {
    // 验证错误响应序列化后不包含 result 字段
    auto resp = MCPResponse::Error(nullptr, -32602, "Invalid params");
    std::string json = glz::write_json(resp).value_or("");
    EXPECT_EQ(json.find("\"result\""), std::string::npos);
}

TEST(MCPToolTest, SuccessResponseErrorFieldNotPresent) {
    // 验证成功响应序列化后不包含 error 字段
    auto resp = MCPResponse::Success(nullptr,
        glz::json_t::object_t{{"ok", true}});
    std::string json = glz::write_json(resp).value_or("");
    EXPECT_EQ(json.find("\"error\""), std::string::npos);
}

} // namespace
