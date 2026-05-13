#pragma once

#include <cstdint>
#include "Export.h"
#include "math/MathTypes.h"

namespace Prisma {

// ============================================================================
// 实体数据池配置
// ============================================================================
static constexpr uint32_t kMaxVirtualEntities = 1024 * 1024; // 1M VA 上限
static constexpr uint32_t kCommitStep = 16384;                // 每次提交 16K 槽位

// Transform 双缓冲 SoA — 字段为指针，指向 VA block 内的固定偏移
struct TransformBufferSoA {
    float* posX;
    float* posY;
    float* rotation;
    float* scaleX;
    float* scaleY;
};

// Render SoA（无缓冲）
struct RenderBufferSoA {
    uint32_t* active;
    uint32_t* generation;
    float*    colorR;
    float*    colorG;
    float*    colorB;
    float*    colorA;
    float*    sizeW;
    float*    sizeH;
};

// ============================================================================
// Node - 轻量级实体句柄
// ============================================================================
// 模仿 C# Node 设计，仅包含一个 uint32_t 句柄。
// 属性访问直接指向底层的 SoA 缓冲区，实现零开销跨语言访问。

struct ENGINE_API Node {
public:
    uint32_t handle = 0;

    Node() = default;
    explicit Node(uint32_t h) : handle(h) {}

    bool IsValid() const;
    void Destroy();

    uint32_t GetIndex() const { return handle & 0xFFFF; }
    uint32_t GetGeneration() const { return handle >> 16; }

    // Transform 属性快捷访问 (自动指向当前活跃的 SoA 缓冲区)
    float GetX() const;
    float GetY() const;
    Vector2 GetPosition() const { return { GetX(), GetY() }; }
    float GetRotation() const;
    Vector2 GetScale() const;

    void SetX(float x);
    void SetY(float y);
    void SetPosition(const Vector2& pos) { SetX(pos.x); SetY(pos.y); }
    void SetRotation(float rot);
    void SetScale(const Vector2& scale);

    // TODO: GetComponent<T>()
    
    bool operator==(const Node& other) const { return handle == other.handle; }
    bool operator!=(const Node& other) const { return handle != other.handle; }
    explicit operator bool() const { return IsValid(); }
};

} // namespace Prisma
