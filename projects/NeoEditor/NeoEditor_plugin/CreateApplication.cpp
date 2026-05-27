#include "NeoEditorPlugin.h"

extern "C" {

GAME_API Prisma::Application* CreateApplication() {
    return new Prisma::NeoEditorPlugin();
}

}
