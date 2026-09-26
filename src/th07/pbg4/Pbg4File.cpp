#include "Pbg4File.hpp"

#include "inttypes.hpp"

// this is like completely worthless
// GLOBAL: TH07 0x00495120
const u32 g_SeekModes[3] = {FILE_BEGIN, FILE_CURRENT, FILE_END};

// would it really not have been simpler to just type the letter where its used
// GLOBAL: TH07 0x0049ea70
const char *g_AccessModes[3] = {
    // STRING: TH07 0x00495244
    "r",
    // STRING: TH07 0x00495240
    "w",
    // STRING: TH07 0x0049523c
    "a",
};

// FUNCTION: TH07 0x0045e550
Pbg4File::Pbg4File()
{
    this->handle = INVALID_HANDLE_VALUE;
    this->access = 0;
}

// FUNCTION: TH07 0x0045e5c0
Pbg4File::~Pbg4File()
{
    Close();
}

#pragma var_order(curModeChar, seekToEnd, pathBuf, actionOnExistingFile)
// FUNCTION: TH07 0x0045e620
bool Pbg4File::Open(const char *path, const char *mode)
{
    DWORD actionOnExistingFile;
    char pathBuf[264];
    i32 seekToEnd;
    const char *curModeChar;

    seekToEnd = 0;
    this->Close();
    for (curModeChar = mode; *curModeChar != '\0'; ++curModeChar)
    {
        if (*curModeChar == 'r')
        {
            this->access = GENERIC_READ;
            actionOnExistingFile = OPEN_EXISTING;
            break;
        }
        if (*curModeChar == 'w')
        {
            DeleteFileA(path);
            this->access = GENERIC_WRITE;
            actionOnExistingFile = CREATE_ALWAYS;
            break;
        }
        if (*curModeChar == 'a')
        {
            seekToEnd = 1;
            this->access = GENERIC_WRITE;
            actionOnExistingFile = OPEN_ALWAYS;
            break;
        }
    }

    if (*curModeChar == '\0')
    {
        return false;
    }

    GetFullPath(pathBuf, path);
    this->handle = CreateFileA(pathBuf, this->access, 1, NULL, actionOnExistingFile,
                               FILE_FLAG_SEQUENTIAL_SCAN | FILE_ATTRIBUTE_NORMAL, NULL);
    if (this->handle == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    if (seekToEnd)
    {
        SetFilePointer(this->handle, 0, NULL, FILE_END);
    }
    return true;
}

// FUNCTION: TH07 0x0045e770
void Pbg4File::Close()
{
    if (this->handle != INVALID_HANDLE_VALUE)
    {
        CloseHandle(this->handle);
        this->handle = INVALID_HANDLE_VALUE;
        this->access = 0;
    }
}

// FUNCTION: TH07 0x0045e7b0
DWORD Pbg4File::Read(void *data, u32 len)
{
    DWORD bytesRead;

    bytesRead = 0;
    if (this->access != GENERIC_READ)
    {
        return 0;
    }

    ReadFile(this->handle, data, len, &bytesRead, NULL);
    return bytesRead;
}

// FUNCTION: TH07 0x0045e800
bool Pbg4File::Write(void *data, u32 len)
{
    DWORD bytesWritten;

    bytesWritten = 0;
    if (this->access != GENERIC_WRITE)
    {
        return false;
    }

    WriteFile(this->handle, data, len, &bytesWritten, NULL);
    return len == bytesWritten ? true : false;
}

// FUNCTION: TH07 0x0045e850
DWORD Pbg4File::Tell()
{
    if (this->handle == INVALID_HANDLE_VALUE)
    {
        return 0;
    }
    else
    {
        return SetFilePointer(this->handle, 0, NULL, FILE_CURRENT);
    }
}

// FUNCTION: TH07 0x0045e880
DWORD Pbg4File::GetSize()
{
    if (this->handle == INVALID_HANDLE_VALUE)
    {
        return 0;
    }
    else
    {
        return GetFileSize(this->handle, NULL);
    }
}

// FUNCTION: TH07 0x0045e8b0
bool Pbg4File::Seek(u32 offset, DWORD seekFrom)
{
    if (this->handle == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    SetFilePointer(this->handle, (LONG)offset, NULL, seekFrom);
    return true;
}

#pragma var_order(buf, fsize, oldPos)
// FUNCTION: TH07 0x0045e8f0
HGLOBAL Pbg4File::ReadRemaining(u32 max)
{
    HGLOBAL buf;
    DWORD fsize;
    DWORD oldPos;

    if (this->access != GENERIC_READ)
    {
        return NULL;
    }

    fsize = this->GetSize();
    if (fsize > max)
    {
        return NULL;
    }

    buf = GlobalAlloc(GMEM_ZEROINIT, fsize);
    if (!buf)
    {
        return NULL;
    }

    oldPos = this->Tell();
    if (!this->Seek(oldPos, g_SeekModes[FILE_BEGIN]))
    {
        return NULL;
    }

    if (this->Read(buf, fsize) == 0)
    {
        if (buf)
        {
            GlobalFree(buf);
            buf = NULL;
        }
        return NULL;
    }

    this->Seek(oldPos, g_SeekModes[FILE_BEGIN]);
    return buf;
}

// FUNCTION: TH07 0x0045e9d0
void Pbg4File::GetFullPath(char *out, const char *filename)
{
    if (strchr(filename, ':') != NULL)
    {
        strcpy(out, filename);
    }
    else
    {
        GetModuleFileNameA(NULL, out, 0x104);
        char *backslashPos = strrchr(out, '\\');
        if (!backslashPos)
        {
            strcpy(out, "");
        }
        backslashPos[1] = '\0';
        strcat(out, filename);
    }
}
