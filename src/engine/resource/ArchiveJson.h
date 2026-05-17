#pragma once
#include "SerializationVersion.h"
#include "Serializable.h"
#include <glaze/glaze.hpp>
#include <glaze/json/generic.hpp>
#include <stack>

namespace Prisma {
namespace Serialization {

using json = glz::json_t;

/**
 * @brief JSON 输出存档
 */
class ENGINE_API JsonOutputArchive : public OutputArchive {
public:
    JsonOutputArchive();
    ~JsonOutputArchive();

    void BeginObject(const std::string& name) override {
        json& current = *m_stack.top();
        if (!current.is_object()) {
            current = glz::json_t::object_t{};
        }
        auto& obj = current.get_object();
        obj[name] = glz::json_t::object_t{};
        m_stack.push(&obj[name]);
    }

    void EndObject() override {
        m_stack.pop();
    }

    void Write(const std::string& key, float value) override { (*m_stack.top()).get_object()[key] = static_cast<double>(value); }
    void Write(const std::string& key, int32_t value) override { (*m_stack.top()).get_object()[key] = static_cast<double>(value); }
    void Write(const std::string& key, uint32_t value) override { (*m_stack.top()).get_object()[key] = static_cast<double>(value); }
    void Write(const std::string& key, uint64_t value) override { (*m_stack.top()).get_object()[key] = static_cast<double>(value); }
    void Write(const std::string& key, bool value) override { (*m_stack.top()).get_object()[key] = value; }
    void Write(const std::string& key, const std::string& value) override { (*m_stack.top()).get_object()[key] = value; }
    
    void Write(const std::string& key, const PrismaMath::vec2& value) override {
        auto& obj = (*m_stack.top()).get_object();
        obj[key] = glz::json_t::array_t{ static_cast<double>(value.x), static_cast<double>(value.y) };
    }
    void Write(const std::string& key, const PrismaMath::vec3& value) override {
        auto& obj = (*m_stack.top()).get_object();
        obj[key] = glz::json_t::array_t{ static_cast<double>(value.x), static_cast<double>(value.y), static_cast<double>(value.z) };
    }
    void Write(const std::string& key, const PrismaMath::vec4& value) override {
        auto& obj = (*m_stack.top()).get_object();
        obj[key] = glz::json_t::array_t{ static_cast<double>(value.x), static_cast<double>(value.y), static_cast<double>(value.z), static_cast<double>(value.w) };
    }
    void Write(const std::string& key, const PrismaMath::quat& value) override {
        auto& obj = (*m_stack.top()).get_object();
        obj[key] = glz::json_t::array_t{ static_cast<double>(value.w), static_cast<double>(value.x), static_cast<double>(value.y), static_cast<double>(value.z) };
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
        if (current.is_object() && current.get_object().contains(name)) {
            m_stack.push(const_cast<json*>(&current.get_object().at(name)));
        } else {
            m_stack.push(m_stack.top());
        }
    }

    void EndObject() override {
        m_stack.pop();
    }

    bool Read(const std::string& key, float& value) override {
        if (m_stack.top()->is_object() && m_stack.top()->get_object().contains(key)) {
            value = static_cast<float>(m_stack.top()->get_object().at(key).get_number());
            return true;
        }
        return false;
    }

    bool Read(const std::string& key, int32_t& value) override {
        if (m_stack.top()->is_object() && m_stack.top()->get_object().contains(key)) {
            value = static_cast<int32_t>(m_stack.top()->get_object().at(key).get_number());
            return true;
        }
        return false;
    }

    bool Read(const std::string& key, uint32_t& value) override {
        if (m_stack.top()->is_object() && m_stack.top()->get_object().contains(key)) {
            value = static_cast<uint32_t>(m_stack.top()->get_object().at(key).get_number());
            return true;
        }
        return false;
    }

    bool Read(const std::string& key, uint64_t& value) override {
        if (m_stack.top()->is_object() && m_stack.top()->get_object().contains(key)) {
            value = static_cast<uint64_t>(m_stack.top()->get_object().at(key).get_number());
            return true;
        }
        return false;
    }

    bool Read(const std::string& key, bool& value) override {
        if (m_stack.top()->is_object() && m_stack.top()->get_object().contains(key)) {
            value = m_stack.top()->get_object().at(key).get_boolean();
            return true;
        }
        return false;
    }

    bool Read(const std::string& key, std::string& value) override {
        if (m_stack.top()->is_object() && m_stack.top()->get_object().contains(key)) {
            value = m_stack.top()->get_object().at(key).get_string();
            return true;
        }
        return false;
    }

    bool Read(const std::string& key, PrismaMath::vec2& value) override {
        if (m_stack.top()->is_object() && m_stack.top()->get_object().contains(key)) {
            const auto& j = m_stack.top()->get_object().at(key).get_array();
            value.x = static_cast<float>(j[0].get_number()); 
            value.y = static_cast<float>(j[1].get_number());
            return true;
        }
        return false;
    }

    bool Read(const std::string& key, PrismaMath::vec3& value) override {
        if (m_stack.top()->is_object() && m_stack.top()->get_object().contains(key)) {
            const auto& j = m_stack.top()->get_object().at(key).get_array();
            value.x = static_cast<float>(j[0].get_number()); 
            value.y = static_cast<float>(j[1].get_number()); 
            value.z = static_cast<float>(j[2].get_number());
            return true;
        }
        return false;
    }

    bool Read(const std::string& key, PrismaMath::vec4& value) override {
        if (m_stack.top()->is_object() && m_stack.top()->get_object().contains(key)) {
            const auto& j = m_stack.top()->get_object().at(key).get_array();
            value.x = static_cast<float>(j[0].get_number()); 
            value.y = static_cast<float>(j[1].get_number()); 
            value.z = static_cast<float>(j[2].get_number()); 
            value.w = static_cast<float>(j[3].get_number());
            return true;
        }
        return false;
    }

    bool Read(const std::string& key, PrismaMath::quat& value) override {
        if (m_stack.top()->is_object() && m_stack.top()->get_object().contains(key)) {
            const auto& j = m_stack.top()->get_object().at(key).get_array();
            value.w = static_cast<float>(j[0].get_number()); 
            value.x = static_cast<float>(j[1].get_number()); 
            value.y = static_cast<float>(j[2].get_number()); 
            value.z = static_cast<float>(j[3].get_number());
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
