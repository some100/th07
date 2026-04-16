#include "FileSystem.hpp"

#include "GameErrorContext.hpp"
#include "dsutil.hpp"
#include "pbg4/Pbg4Archive.hpp"

// GLOBAL: TH07 0x004b9e64
u32 g_LastFileSize;

// FUNCTION: TH07 0x00431330
u8 *FileSystem::OpenFile(const char *filepath, i32 isExternalResource)
{
    u8 *buf;
    HANDLE hFile;
    DWORD fsize;
    const char *filename;

    i32 entryIdx = -1;
    if (isExternalResource == 0)
    {
        filename = strrchr(filepath, '\\');
        if (filename == NULL)
            filename = filepath;
        else
            filename = filename + 1;

        filename = strrchr(filename, '/');
        if (filename == NULL)
            filename = filepath;
        else
            filename = filename + 1;
        fsize = g_Pbg4Archive.GetEntrySize(filename);
        g_LastFileSize = fsize;
        if (fsize == 0)
        {
            // STRING: TH07 0x00497d38
            g_GameErrorContext.Fatal("error : %s is not found in arcfile.\r\n",
                                     filename);
            return NULL;
        }
        if (fsize != 0)
        {
            // STRING: TH07 0x00497d24
            DebugPrint("%s Decode ... \r\n", filename);
            buf = (u8 *)malloc(fsize);
            if (buf == NULL)
                return NULL;

            g_Pbg4Archive.ReadDecompressEntry(filename, buf);
            return buf;
        }
    }
    // STRING: TH07 0x00497d14
    DebugPrint("%s Load ... \r\n", filepath);
    hFile = CreateFileA(filepath, GENERIC_READ, 1, NULL, 3, 0x8000080, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        // STRING: TH07 0x00497cf8
        DebugPrint("error : %s is not found.\r\n", filepath);
        return NULL;
    }

    fsize = GetFileSize(hFile, NULL);
    buf = (u8 *)malloc(fsize);
    if (buf == NULL)
    {
        CloseHandle(hFile);
        return NULL;
    }
    ReadFile(hFile, buf, fsize, &fsize, NULL);
    g_LastFileSize = fsize;
    CloseHandle(hFile);
    return buf;
}

// FUNCTION: TH07 0x004314f0
i32 FileSystem::CheckFileExists(const char *file)
{
    HANDLE hObject;

    hObject = CreateFileA(file, GENERIC_READ, 1, NULL, 3, 0x8000080, NULL);
    if (hObject != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hObject);
        return true;
    }
    return false;
}

#pragma var_order(bytesWritten, hFile)
// FUNCTION: TH07 0x00431540
i32 FileSystem::WriteDataToFile(const char *filename, const void *out,
                                DWORD bytesToWrite)
{
    HANDLE hFile;
    DWORD bytesWritten;

    hFile = CreateFileA(filename, GENERIC_WRITE, 1, NULL, 2, 0x80, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        // STRING: TH07 0x00497cdc
        DebugPrint("error : %s write error\r\n", filename);
        return -1;
    }
    WriteFile(hFile, out, bytesToWrite, &bytesWritten, NULL);
    if (bytesToWrite != bytesWritten)
    {
        CloseHandle(hFile);
        DebugPrint("error : %s write error\r\n", filename);
        return -2;
    }
    CloseHandle(hFile);
    // STRING: TH07 0x00497ccc
    DebugPrint("%s write ...\r\n", filename);
    return 0;
}
