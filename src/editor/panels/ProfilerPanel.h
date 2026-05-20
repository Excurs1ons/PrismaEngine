#pragma once

namespace Prisma {

class ProfilerPanel {
public:
    static void OnImGuiRender();
    static void Toggle() { s_show = !s_show; }
    static bool IsVisible() { return s_show; }

private:
    static inline bool s_show = false;
};

}
