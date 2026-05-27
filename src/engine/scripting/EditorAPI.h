#pragma once

#include <cstdint>
#include "Export.h"

namespace Prisma {
namespace Scripting {

struct EditorAPI {
    // --- Scene ---
    void*       (*sceneGetHierarchy)();
    uint64_t    (*sceneCreateEntity)(const char* name);
    bool        (*sceneDeleteEntity)(uint64_t id);
    bool        (*sceneRenameEntity)(uint64_t id, const char* name);

    // --- Transform ---
    void        (*transformGetPosition)(uint64_t id, float* outX, float* outY, float* outZ);
    void        (*transformSetPosition)(uint64_t id, float x, float y, float z);
    void        (*transformGetRotation)(uint64_t id, float* outX, float* outY, float* outZ, float* outW);
    void        (*transformSetRotation)(uint64_t id, float x, float y, float z, float w);
    void        (*transformGetScale)(uint64_t id, float* outX, float* outY, float* outZ);
    void        (*transformSetScale)(uint64_t id, float x, float y, float z);

    // --- Selection ---
    uint64_t    (*selectionGetSelected)();
    void        (*selectionSetSelected)(uint64_t id);
    void        (*selectionClear)();

    // --- Viewport ---
    void        (*viewportGetSize)(uint32_t* outW, uint32_t* outH);
    void        (*viewportSetSize)(uint32_t w, uint32_t h);
    bool        (*viewportCaptureScreenshot)(const char* path);

    // --- Asset ---
    void*       (*assetBrowseDirectory)(const char* path);
    void*       (*assetGetInfo)(const char* path);
    bool        (*assetImport)(const char* sourcePath);

    // --- Editor ---
    void*       (*editorGetStatus)();
    void*       (*editorGetEngineInfo)();
    bool        (*editorExecuteCommand)(const char* cmd);

    // --- Log ---
    void*       (*logGetLogs)(int level, int count);
    void        (*logClear)();

    // === 内存管理 ===
    // 所有返回 void* (JSON string) 的函数使用 strdup 分配内存，
    // C# 侧通过此函数释放
    void        (*freeString)(void* ptr);

    // 结构体大小，用于 C++/C# 版本校验
    // C++ 侧在 FillEditorAPI 中设置为 sizeof(EditorAPI)
    // C# 侧在 Init 中校验，不匹配时抛出异常
    uint32_t structSize;
};

void FillEditorAPI(EditorAPI& api);

} // namespace Scripting
} // namespace Prisma

extern "C" {

ENGINE_API void GetEditorAPI(Prisma::Scripting::EditorAPI* outApi);

}
