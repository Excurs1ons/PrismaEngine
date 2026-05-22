#pragma once

#include "Export.h"
#include "core/Component.h"
#include <string>
#include <glaze/glaze.hpp>

namespace Prisma::Scripting {

/// @brief C# 脚本组件桥接类
/// 允许在 C++ Scene 序列化中保存 C# Script 的类名和字段值，
/// 序列化格式：{"scriptClass":"...", "fields":{...}}
/// 等 C# ScriptEngine 加载时解析 fields JSON 创建实际 Script 实例。
class ENGINE_API ScriptComponent : public Component {
public:
    struct Data {
        std::string scriptClass;            // C# 脚本类名（如 "CameraController3D"）
        glz::raw_json fields;               // 字段值的原始 JSON（如 {"moveSpeed":5.0}）
    };

    ScriptComponent() = default;
    ~ScriptComponent() override;

    // Component 接口
    void Initialize() override {}
    void Update([[maybe_unused]] Timestep ts) override {}
    void Shutdown() override {}

    // 类型标识
    const char* GetComponentTypeName() const override { return "Script"; }

    // 序列化数据
    Data GetData() const;
    void SetData(const Data& d);

    // 字段访问器
    const std::string& GetScriptClass() const { return m_scriptClass; }
    void SetScriptClass(const std::string& cls) { m_scriptClass = cls; }

    const std::string& GetFieldsJson() const { return m_fields.str; }
    void SetFieldsJson(const std::string& json) { m_fields = glz::raw_json{json}; }

private:
    std::string m_scriptClass;
    glz::raw_json m_fields;
};

} // namespace Prisma::Scripting
