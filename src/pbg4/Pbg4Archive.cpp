#include "Pbg4Archive.hpp"

#include <SDL3/SDL_stdinc.h>
#include <cstring>

#include "Lzss.hpp"
#include "Pbg4File.hpp"
#include "Supervisor.hpp"
#include "dxutil.hpp"

Pbg4Archive g_UnusedPbg4ArchiveArray[20];

Pbg4Archive g_Pbg4Archive;

Pbg4Archive::Pbg4Archive()
{
    this->entries = NULL;
    this->numOfEntries = 0;
    this->filename = NULL;
    this->fileAbstraction = NULL;
}

Pbg4Archive::~Pbg4Archive()
{
    Release();
}

bool Pbg4Archive::Load(const char *filename)
{
    Release();
    Supervisor::DebugPrint("info : %s open arcfile\n", filename);

    this->fileAbstraction = new Pbg4File();

    if (!this->fileAbstraction)
    {
        return false;
    }

    if (OpenArchive(filename))
    {
        this->filename = CopyFileName(filename);
        if (this->filename)
        {
            return true;
        }
    }
    Supervisor::DebugPrint("info : %s not found\n", filename);
    Release();
    return false;
}

void Pbg4Archive::Release()
{
    Supervisor::DebugPrint("info : %s close arcfile\n", this->filename);
    if (this->filename)
    {
        free(this->filename);
        this->filename = NULL;
    }
    SAFE_DELETE_ARRAY(this->entries);
    SAFE_DELETE(this->fileAbstraction);
    this->numOfEntries = 0;
}

u8 *Pbg4Archive::ReadDecompressEntry(const char *filename, u8 *buf)
{
    size_t dstLen;
    Pbg4Entry *entry;
    size_t dwBytes;
    u8 *dstBuf;
    u8 *srcBuf;

    srcBuf = NULL;
    if (!this->fileAbstraction)
    {
        return NULL;
    }

    entry = FindEntry(filename);
    if (!entry)
    {
        goto err;
    }

    if (this->fileAbstraction->Open(this->filename, g_AccessModes[0]) == 0)
    {
        goto err;
    }

    dwBytes = entry[1].dataOffset - entry->dataOffset;
    dstLen = entry->decompressedSize;
    srcBuf = (u8 *)malloc(dwBytes);
    if (!srcBuf)
    {
        goto err;
    }

    if (!this->fileAbstraction->Seek(entry->dataOffset, g_SeekModes[0]))
    {
        goto err;
    }
    if (this->fileAbstraction->Read(srcBuf, dwBytes) == 0)
    {
        goto err;
    }

    dstBuf = Lzss::Decompress(srcBuf, dwBytes, buf, dstLen);
    if (srcBuf)
    {
        free(srcBuf);
        srcBuf = NULL;
    }
    return dstBuf;
err:
    Supervisor::DebugPrint("info : %s error\n", this->filename);
    if (srcBuf)
    {
        free(srcBuf);
        srcBuf = NULL;
    }
    return NULL;
}

u32 Pbg4Archive::GetEntrySize(const char *filename)
{
    Pbg4Entry *entry = FindEntry(filename);

    if (entry)
    {
        return entry->decompressedSize;
    }
    return 0;
}

Pbg4Entry *Pbg4Archive::FindEntry(const char *filename)
{
    if (!this->entries)
    {
        return NULL;
    }

    Pbg4Entry *entry = this->entries;
    for (i32 i = this->numOfEntries; 0 < i; --i, ++entry)
    {
        if (SDL_strcasecmp(filename, entry->filename) == 0)
        {
            return entry;
        }
    }
    return NULL;
}

bool Pbg4Archive::OpenArchive(const char *path)
{
    u8 *compressedData;
    u32 headerSize;
    u32 fileSize;
    i32 magic;
    u32 decompressedSize;
    u8 *decompressedData;

    compressedData = NULL;
    decompressedData = NULL;
    if (!this->fileAbstraction)
    {
        return false;
    }
    if (!this->fileAbstraction->Open(path, g_AccessModes[0]))
    {
        goto err;
    }

    if (ReadFile(&magic, 4) == 0)
    {
        goto err;
    }
    if (magic != '4GBP')
    {
        goto err;
    }

    if (ReadFile(&this->numOfEntries, 4) == 0)
    {
        goto err;
    }
    if (this->numOfEntries <= 0)
    {
        goto err;
    }

    fileSize = this->fileAbstraction->GetSize();

    if (ReadFile(&headerSize, 4) == 0)
    {
        goto err;
    }
    if (headerSize >= fileSize)
    {
        goto err;
    }

    fileSize -= headerSize;

    if (ReadFile(&decompressedSize, 4) == 0)
    {
        goto err;
    }

    this->fileAbstraction->Seek(headerSize, g_SeekModes[0]);
    compressedData = (u8 *)malloc(fileSize);
    if (!compressedData)
    {
        goto err;
    }

    if (this->fileAbstraction->Read(compressedData, fileSize) == 0)
    {
        goto err;
    }

    decompressedData = Lzss::Decompress(compressedData, fileSize, NULL, decompressedSize);
    if (!decompressedData)
    {
        goto err;
    }

    this->entries = AllocEntries(decompressedData, this->numOfEntries, headerSize);
    if (!this->entries)
    {
        goto err;
    }

    if (compressedData)
    {
        free(compressedData);
        compressedData = NULL;
    }
    if (decompressedData)
    {
        free(decompressedData);
        decompressedData = NULL;
    }
    return true;
err:
    if (compressedData)
    {
        free(compressedData);
        compressedData = NULL;
    }
    if (decompressedData)
    {
        free(decompressedData);
        decompressedData = NULL;
    }
    SAFE_DELETE(this->fileAbstraction);
    Supervisor::DebugPrint("ファイル %s のオープン中にエラーが発生しました\n", path);
    return false;
}

Pbg4Entry *Pbg4Archive::AllocEntries(void *data, i32 count, u32 dataOffset)
{
    Pbg4Entry *entries = NULL;
    i32 i;
    u8 *entryData;

    entries = new Pbg4Entry[count + 1];

    if (!entries)
    {
        goto err;
    }

    entryData = (u8 *)data;
    for (i = 0; i < count; i++)
    {
        entries[i].filename = CopyFileName((char *)entryData);

        entryData += strlen((char *)entryData) + 1;

        memcpy(&entries[i].dataOffset, entryData, 4);
        entryData += 4;

        memcpy(&entries[i].decompressedSize, entryData, 4);
        entryData += 4;

        memcpy(&entries[i].magicThing, entryData, 4);
        entryData += 4;
    }

    entries[count].dataOffset = dataOffset;
    entries[count].decompressedSize = 0;

    return entries;

err:
    SAFE_DELETE_ARRAY(entries);
    return NULL;
}

char *Pbg4Archive::CopyFileName(const char *filename)
{
    char *pcVar2;

    pcVar2 = (char *)malloc(strlen(filename) + 1);
    if (pcVar2)
    {
        strcpy(pcVar2, filename);
    }
    return pcVar2;
}
