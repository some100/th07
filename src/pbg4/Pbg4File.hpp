#pragma once

#include <windows.h>

#include "../inttypes.hpp"

// VTABLE: TH07 0x0049526c
struct IPbg4File
{
    IPbg4File()
    {
    }

    // SYNTHETIC: TH07 0x0047e90b
    // _purecall

    virtual bool Open(const char *path, const char *mode) = 0;
    virtual void Close() = 0;
    virtual DWORD Read(void *data, u32 len) = 0;
    virtual bool Write(void *data, u32 len) = 0;
    virtual DWORD Tell() = 0;
    virtual DWORD GetSize() = 0;
    virtual bool Seek(u32 offset, DWORD seekFrom) = 0;
    virtual ~IPbg4File()
    {
    }

    // SYNTHETIC: TH07 0x0045e520
    // IPbg4File::`scalar deleting destructor'
};

// VTABLE: TH07 0x00495248
struct Pbg4File : IPbg4File
{
    Pbg4File();
    virtual ~Pbg4File();
    // SYNTHETIC: TH07 0x0045e590
    // Pbg4File::`scalar deleting destructor'

    virtual bool Open(const char *path, const char *mode);
    virtual void Close();
    virtual DWORD Read(void *data, u32 len);
    virtual bool Write(void *data, u32 len);
    virtual DWORD Tell();
    virtual DWORD GetSize();
    virtual bool Seek(u32 offset, DWORD seekFrom);
    virtual HGLOBAL ReadRemaining(u32 max);

    static void GetFullPath(char *out, const char *filename);

    HANDLE handle;
    u32 access;
};

extern u32 g_SeekModes[3];
extern const char *g_AccessModes[3];
