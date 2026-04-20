#pragma once

namespace ZunMemory
{
inline void Free(void *p)
{
    free(p);
}

inline void *Alloc(u32 size)
{
    return malloc(size);
}
} // namespace ZunMemory
