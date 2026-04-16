#pragma once

#include <windows.h>

#include "inttypes.hpp"

namespace FileSystem
{
i32 CheckFileExists(const char *file);
u8 *OpenFile(const char *filepath, i32 isExternalResource);
i32 WriteDataToFile(const char *filename, const void *out, DWORD bytesToWrite);
} // namespace FileSystem

extern u32 g_LastFileSize;
