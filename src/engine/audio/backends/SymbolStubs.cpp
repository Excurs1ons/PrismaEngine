// stb_vorbis implementation (single compilation unit)
#define STB_VORBIS_IMPLEMENTATION
#include <stb_vorbis.c>

#if defined(_MSC_VER) && defined(_M_X64)
#include <intrin.h>
extern "C" void Prisma_FastStoreFence() {
    _mm_sfence();
}
#endif
