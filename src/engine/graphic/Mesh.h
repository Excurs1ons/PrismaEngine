
#pragma once

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif

#include "AssetBase.h"
#include "MathTypes.h"
#include "Handle.h"
#include "graphic/SubMesh.h"
#include "graphic/interfaces/RenderTypes.h"
#include <nlohmann/json.hpp>
using namespace PrismaEngine;
using namespace PrismaEngine::Graphic;
class Mesh : public PrismaEngine::AssetBase
{
    // 加载到GPU的方法
    //bool UploadToGPU(RenderDevice* device);
    //void Render(CommandBuffer* cmd, MaterialInstance* materialOverrides = nullptr);
	/// @brief 获取顶点数据的字节大小
public:
    std::vector<SubMesh> subMeshes;
    BoundingBox globalBoundingBox; // 整个Mesh的包围盒
    bool keepCpuData = false;
	static Mesh GetCubeMesh();
    static Mesh GetTriangleMesh();
	static Mesh GetQuadMesh();

    [[nodiscard]] PrismaEngine::AssetType GetType() const override {
        return PrismaEngine::AssetType::Mesh;
    }
    bool Load(const std::filesystem::path& path) override;
    void Unload() override;
    bool IsLoaded() const override;

private:
    bool m_isLoaded = false;
};
