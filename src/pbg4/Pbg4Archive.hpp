#pragma once

#include <cstdlib>

#include "Pbg4File.hpp"
#include "inttypes.hpp"

struct Pbg4Entry
{
    Pbg4Entry()
    {
        filename = NULL;
    }

    ~Pbg4Entry()
    {
        if (filename)
        {
            free(filename);
            filename = NULL;
        }
    }

    // SYNTHETIC: TH07 0x0045f8d0
    // Pbg4Entry::`vector deleting destructor'

    char *filename;
    u32 dataOffset;
    u32 decompressedSize;
    u32 magicThing;
};

struct Pbg4Archive
{
    Pbg4Archive();
    ~Pbg4Archive();
    Pbg4Entry *AllocEntries(void *data, i32 count, u32 dataOffset);
    char *CopyFileName(const char *filename);
    Pbg4Entry *FindEntry(const char *filename);
    u32 GetEntrySize(const char *filename);
    bool Load(const char *filename);
    bool OpenArchive(const char *path);
    u8 *ReadDecompressEntry(const char *filename, u8 *buf);
    void Release();

    u32 ReadFile(void *data, u32 len)
    {
        Pbg4File *file = this->fileAbstraction;
        return file->Read(data, len);
    }

    Pbg4Entry *entries;
    i32 numOfEntries;
    char *filename;
    Pbg4File *fileAbstraction;
};
extern Pbg4Archive g_Pbg4Archive;
