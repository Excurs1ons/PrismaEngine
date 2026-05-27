#if defined(_MSC_VER) && defined(_M_X64)
#include <intrin.h>
#pragma intrinsic(_mm_sfence)
void __faststorefence()
{
    _mm_sfence();
}
#endif
