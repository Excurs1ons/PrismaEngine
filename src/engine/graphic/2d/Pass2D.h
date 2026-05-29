#pragma once

#include "graphic/LogicalPass.h"

namespace Prisma::Graphic {

/**
 * @brief 2D Pass 基类 — 替代 ForwardRenderPass 在 2D 管线中的角色
 * 
 * 不携带 view/projection 矩阵（那是 3D 概念）。
 * 2D 渲染使用 ortho MVP，由各 Pass 自行管理。
 */
class Pass2D : public LogicalPass {
public:
    Pass2D(const char* name) : LogicalPass(name) {}
    ~Pass2D() override = default;
};

} // namespace Prisma::Graphic
