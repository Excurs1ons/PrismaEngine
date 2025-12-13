//
// Created by JasonGu on 25-12-14.
//

#ifndef HANDLE_H
#define HANDLE_H

// 基础句柄类型
class Handle {
private:
    std::optional<uint32_t> m_id;

public:
    Handle() = default;
    explicit Handle(uint32_t id) : m_id(id) {}

    bool IsValid() const { return m_id.has_value(); }
    explicit operator bool() const { return IsValid(); }

    uint32_t GetId() const { return m_id.value_or(0xFFFFFFFF); }

    static Handle Invalid() { return Handle(); }
};

// 类型安全的派生句柄
struct VertexBufferHandle : public Handle {};
struct IndexBufferHandle : public Handle {};
struct TextureHandle : public Handle {};




#endif //HANDLE_H
