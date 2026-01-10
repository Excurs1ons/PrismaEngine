#ifndef ANDROIDGLINVESTIGATIONS_MODEL_H
#define ANDROIDGLINVESTIGATIONS_MODEL_H

#include "Mesh.h"

#ifdef PRISMA_ENABLE_RENDER_VULKAN
#include "TextureAsset.h"
#else
#include "resource/TextureAsset.h"
#endif

#include "graphic/interfaces/RenderTypes.h"
#include "math/MathTypes.h"
#include <vector>

using namespace PrismaEngine;

typedef uint16_t Index;

class Model {
public:
    inline Model(
            std::vector<Vertex> vertices,
            std::vector<Index> indices,
            std::shared_ptr<TextureAsset> spTexture = nullptr)
            : vertices_(std::move(vertices)),
              indices_(std::move(indices)),
              spTexture_(std::move(spTexture)) {}

    inline const Vertex *getVertexData() const {
        return vertices_.data();
    }

    inline const size_t getVertexCount() const {
        return vertices_.size();
    }

    inline const size_t getIndexCount() const {
        return indices_.size();
    }

    inline const Index *getIndexData() const {
        return indices_.data();
    }

    // 检查是否有纹理（如果没有纹理，需要使用 fallback 纹理）
    inline bool hasTexture() const {
        return spTexture_ != nullptr;
    }

    // 获取纹理（要求必须有纹理）
    inline const TextureAsset &getTexture() const {
        return *spTexture_;
    }

    // 获取纹理指针（可以为 nullptr）
    inline const std::shared_ptr<TextureAsset>& getTexturePtr() const {
        return spTexture_;
    }

private:
    std::vector<Vertex> vertices_;
    std::vector<Index> indices_;
    std::shared_ptr<TextureAsset> spTexture_;
};

#endif //ANDROIDGLINVESTIGATIONS_MODEL_H