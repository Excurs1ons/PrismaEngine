#pragma once

#include "../Export.h"

namespace Prisma {
namespace Profiling {

// ─── ProfilerPanel ──────────────────────────────────────────────────────────
// ImGui panel that visualises profiling data from ProfilerSystem.
// Toggle with Toggle() / keyboard shortcut.
//
// NOTE: This panel requires Dear ImGui.  When compiled into a context that
// does not have ImGui (e.g. the Engine library), OnImGuiRender() is a no-op.
//
class ENGINE_API ProfilerPanel {
public:
    static void OnImGuiRender();
    static void Toggle();
    static bool IsVisible() { return s_show; }

private:
    static inline bool s_show = false;
};

} // namespace Profiling
} // namespace Prisma
