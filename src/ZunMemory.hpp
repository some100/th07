#pragma once

#include <stdlib.h>

namespace ZunMemory
{
inline void Free(void *p)
{
    free(p);
}

inline void *Alloc(size_t size)
{
    return malloc(size);
}
} // namespace ZunMemory
