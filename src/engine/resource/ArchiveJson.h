#pragma once
#include "SerializationVersion.h"
#include "Serializable.h"
#include <nlohmann/json.hpp>
#include <stack>

namespace Prisma {
namespace Serialization {

using json = nlohmann::json;

/**
 * @brief JSON 输出存档
 */
class ENGINE_API JsonOutputArchive : public OutputArchive {
public:
    JsonOutputArchive();
    ~JsonOutputArchive();

    void BeginObject(const std::string& name) override {
        json& current = *m_stack.top();
        current[name] = json::object();
        m_stack.push(&current[name]);
    }

    void EndObject() override {
        m_stack.pop();
    }

    void Write(const std::string& key, float value) override { (*m_stack.top())[key] = value; }
    void Write(const std::string& key, int32_t value) override { (*m_stack.top())[key] = value; }
    void Write(const std::string& key, uint32_t value) override { (*m_stack.top())[key] = value; }
    void Write(const std::string& key, bool value) override { (*m_stack.top())[key] = value; }
    void Write(const std::string& key, const std::string& value) override { (*m_stack.top())[key] = value; }
    
    void Write(const std::string& key, const PrismaMath::vec2& value) override {
        (*m_stack.top())[key] = { value.x, value.y };
    }
    void Write(const std::string& key, const PrismaMath::vec3& value) override {
        (*m_stack.top())[key] = { value.x, value.y, value.z };
    }
    void Write(const std::string& key, const PrismaMath::vec4& value) override {
        (*m_stack.top())[key] = { value.x, value.y, value.z, value.w };
    }
    void Write(const std::string& key, const PrismaMath::quat& value) override {
        (*m_stack.top())[key] = { value.w, value.x, value.y, value.z };
    }

    const json& GetJson() const { return m_root; }

private:
    json m_root;
    std::stack<json*> m_stack;
};

/**
 * @brief JSON 输入存档
 */
class ENGINE_API JsonInputArchive : public InputArchive {
public:
    explicit JsonInputArchive(const json& data);
    ~JsonInputArchive();

    void BeginObject(const std::string& name) override {
        const json& current = *m_stack.top();
        if (current.contains(name)) {
            m_stack.push(const_cast<json*>(&current.at(name)));
        } else {
            // Push same to keep stack balanced, but mark as invalid or handle
            m_stack.push(m_stack.top());
        }
    }

    void EndObject() override {
        m_stack.pop();
    }

    bool Read(const std::string& key, float& value) override {
        if (m_stack.top()->contains(key)) {
            value = m_stack.top()->at(key).get<float>();
            return true;
        }
        return false;
    }

    bool Read(const std::string& key, int32_t& value) override {
        if (m_stack.top()->contains(key)) {
            value = m_stack.top()->at(key).get<int32_t>();
            return true;
        }
        return false;
    }

    bool Read(const std::string& key, uint32_t& value) override {
        if (m_stack.top()->contains(key)) {
            value = m_stack.top()->at(key).get<uint32_t>();
            return true;
        }
        return false;
    }

    bool Read(const std::string& key, bool& value) override {
        if (m_stack.top()->contains(key)) {
            value = m_stack.top()->at(key).get<bool>();
            return true;
        }
        return false;
    }

    bool Read(const std::string& key, std::string& value) override {
        if (m_stack.top()->contains(key)) {
            value = m_stack.top()->at(key).get<std::string>();
            return true;
        }
        return false;
    }

    bool Read(const std::string& key, PrismaMath::vec2& value) override {
        if (m_stack.top()->contains(key)) {
            const auto& j = m_stack.top()->at(key);
            value.x = j[0]; value.y = j[1];
            return true;
        }
        return false;
    }

    bool Read(const std::string& key, PrismaMath::vec3& value) override {
        if (m_stack.top()->contains(key)) {
            const auto& j = m_stack.top()->at(key);
            value.x = j[0]; value.y = j[1]; value.z = j[2];
            return true;
        }
        return false;
    }

    bool Read(const std::string& key, PrismaMath::vec4& value) override {
        if (m_stack.top()->contains(key)) {
            const auto& j = m_stack.top()->at(key);
            value.x = j[0]; value.y = j[1]; value.z = j[2]; value.w = j[3];
            return true;
        }
        return false;
    }

    bool Read(const std::string& key, PrismaMath::quat& value) override {
        if (m_stack.top()->contains(key)) {
            const auto& j = m_stack.top()->at(key);
            value.w = j[0]; value.x = j[1]; value.y = j[2]; value.z = j[3];
            return true;
        }
        return false;
    }

private:
    const json& m_root;
    std::stack<json*> m_stack;
};

} // namespace Serialization
} // namespace Prisma
