#pragma once

#include "Logger.h"
#include "LogScope.h"

// 简化的错误处理宏，用于减少重复的日志作用域清理代码
#define HANDLE_RENDER_ERROR(errorMessage, frameScope) \
    do { \
        LOG_ERROR("RendererVulkan", errorMessage); \
        if (frameScope) { \
            Logger::GetInstance().PopLogScope(frameScope); \
            LogScopeManager::GetInstance().DestroyScope(frameScope, false); \
        } \
        return; \
    } while(0)

#define HANDLE_RENDER_ERROR_WITH_RETURN(errorMessage, frameScope, returnValue) \
    do { \
        LOG_ERROR("RendererVulkan", errorMessage); \
        if (frameScope) { \
            Logger::GetInstance().PopLogScope(frameScope); \
            LogScopeManager::GetInstance().DestroyScope(frameScope, false); \
        } \
        return returnValue; \
    } while(0)

// Vulkan结果检查宏
#define CHECK_VK_RESULT(result, errorMessage, frameScope) \
    do { \
        if (result != VK_SUCCESS) { \
            LOG_ERROR("RendererVulkan", errorMessage ": {0}", static_cast<int>(result)); \
            if (frameScope) { \
                Logger::GetInstance().PopLogScope(frameScope); \
                LogScopeManager::GetInstance().DestroyScope(frameScope, false); \
            } \
            return; \
        } \
    } while(0)