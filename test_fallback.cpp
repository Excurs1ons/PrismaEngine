#include <iostream>
#include "src/engine/resource/ResourceManager.h"
#include "src/engine/graphic/Shader.h"
#include "src/engine/graphic/Material.h"
#include "src/engine/graphic/Mesh.h"

int main() {
    // 初始化日志系统
    Engine::LogConfig logConfig;
    logConfig.logFilePath = "test_fallback.log";
    logConfig.minLevel = Engine::LogLevel::Info;
    Engine::Logger::GetInstance().Initialize(logConfig);

    // 创建资源管理器
    auto resourceManager = Engine::ResourceManager::GetInstance();
    resourceManager->Initialize(".");

    // 测试着色器 fallback
    std::cout << "测试着色器 fallback..." << std::endl;
    auto shaderHandle = resourceManager->Load<Engine::Shader>("nonexistent_shader.hlsl");
    if (shaderHandle.IsValid()) {
        std::cout << "✓ 成功使用默认着色器作为回退" << std::endl;
    } else {
        std::cout << "✗ 着色器 fallback 失败" << std::endl;
    }

    // 测试网格 fallback
    std::cout << "\n测试网格 fallback..." << std::endl;
    auto meshHandle = resourceManager->Load<Engine::Mesh>("nonexistent_mesh.mesh");
    if (meshHandle.IsValid()) {
        std::cout << "✓ 成功使用默认网格作为回退" << std::endl;
    } else {
        std::cout << "✗ 网格 fallback 失败" << std::endl;
    }

    // 测试材质 fallback
    std::cout << "\n测试材质 fallback..." << std::endl;
    auto materialHandle = resourceManager->Load<Engine::Material>("nonexistent_material.mat");
    if (materialHandle.IsValid()) {
        std::cout << "✓ 成功使用默认材质作为回退" << std::endl;
    } else {
        std::cout << "✗ 材质 fallback 失败" << std::endl;
    }

    std::cout << "\n测试完成！" << std::endl;
    return 0;
}