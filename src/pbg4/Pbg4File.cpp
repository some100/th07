#include "Pbg4File.hpp"

#include "../inttypes.hpp"

// GLOBAL: TH07 0x00495120
u32 g_SeekModes[3] = {0, 1, 2};

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

#pragma var_order(local_8, local_c, local_114, local_118)
// FUNCTION: TH07 0x0045e620
bool Pbg4File::Open(const char *path, const char *mode)
{
    DWORD local_118;
    char local_114[264];
    i32 local_c;
    const char *local_8;

    local_c = 0;
    this->Close();
    for (local_8 = mode; *local_8 != '\0'; ++local_8)
    {
        if (*local_8 == 'r')
        {
            this->access = GENERIC_READ;
            local_118 = 3;
            break;
        }
        if (*local_8 == 'w')
        {
            DeleteFileA(path);
            this->access = GENERIC_WRITE;
            local_118 = 2;
            break;
        }
        if (*local_8 == 'a')
        {
            local_c = 1;
            this->access = GENERIC_WRITE;
            local_118 = 4;
            break;
        }
    }
    if (*local_8 == '\0')
    {
        return false;
    }
    else
    {
        GetFullPath(local_114, path);
        this->handle = CreateFileA(local_114, this->access, 1, NULL, local_118,
                                   0x8000080, NULL);
        if (this->handle == INVALID_HANDLE_VALUE)
        {
            return false;
        }
        else
        {
            if (local_c != 0)
            {
                SetFilePointer(this->handle, 0, NULL, 2);
            }
            return true;
        }
    }
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
    DWORD local_8;

    local_8 = 0;
    if (this->access != GENERIC_READ)
        return 0;

    ReadFile(this->handle, data, len, &local_8, NULL);
    return local_8;
}

// FUNCTION: TH07 0x0045e800
bool Pbg4File::Write(void *data, u32 len)
{
    DWORD local_8;

    local_8 = 0;
    if (this->access != GENERIC_WRITE)
        return false;

    WriteFile(this->handle, data, len, &local_8, NULL);
    return len == local_8 ? true : false;
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
        return SetFilePointer(this->handle, 0, NULL, 1);
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
        return false;

    SetFilePointer(this->handle, (LONG)offset, NULL, seekFrom);
    return true;
}

#pragma var_order(hMem, DVar2, DVar3)
// FUNCTION: TH07 0x0045e8f0
HGLOBAL Pbg4File::ReadRemaining(u32 max)
{
    HGLOBAL hMem;
    DWORD DVar2;
    DWORD DVar3;

    if (this->access != GENERIC_READ)
        return NULL;

    DVar2 = this->GetSize();
    if (DVar2 > max)
        return NULL;

    hMem = GlobalAlloc(0x40, DVar2);
    if (hMem == NULL)
        return NULL;

    DVar3 = this->Tell();
    if (!this->Seek(DVar3, g_SeekModes[0]))
        return NULL;

    if (this->Read(hMem, DVar2) == 0)
    {
        if (hMem != NULL)
        {
            GlobalFree(hMem);
            hMem = NULL;
        }
        return NULL;
    }

    this->Seek(DVar3, g_SeekModes[0]);
    return hMem;
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
        char *pcVar2 = strrchr(out, '\\');
        if (pcVar2 == NULL)
        {
            strcpy(out, "");
        }
        pcVar2[1] = '\0';
        strcat(out, filename);
    }
}
