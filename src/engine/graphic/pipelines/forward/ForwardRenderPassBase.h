//
// Created by JasonGu on 26-1-1.
//

#ifndef FORWARDRENDERPASSBASE_H
#define FORWARDRENDERPASSBASE_H

#pragma once
#include "LogicalPass.h"
namespace PrismaEngine::Graphic {
/// @brief 前向渲染 Pass 基类
/// 用于前向渲染管线中的 Pass
class ForwardRenderPass : public LogicalPass {
public:
    ForwardRenderPass(const char* name);
    ~ForwardRenderPass() override = default;

    // === 相机数据 ===

    /// @brief 设置视图矩阵
    void SetViewMatrix(const PrismaMath::mat4& view) { m_view = view; UpdateViewProjection(); }

    /// @brief 设置投影矩阵
    void SetProjectionMatrix(const PrismaMath::mat4& projection) { m_projection = projection; UpdateViewProjection(); }

    /// @brief 设置视图投影矩阵
    void SetViewProjectionMatrix(const PrismaMath::mat4& viewProjection) { m_viewProjection = viewProjection; }

    /// @brief 获取视图矩阵
    const PrismaMath::mat4& GetViewMatrix() const { return m_view; }

    /// @brief 获取投影矩阵
    const PrismaMath::mat4& GetProjectionMatrix() const { return m_projection; }

    /// @brief 获取视图投影矩阵
    const PrismaMath::mat4& GetViewProjectionMatrix() const { return m_viewProjection; }

protected:
    void UpdateViewProjection() { m_viewProjection = m_projection * m_view; }

    PrismaMath::mat4 m_view;
    PrismaMath::mat4 m_projection;
    PrismaMath::mat4 m_viewProjection;
};
}




#endif //FORWARDRENDERPASSBASE_H
