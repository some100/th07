#include "Pbg4Archive.hpp"

#include "Lzss.hpp"
#include "Pbg4File.hpp"
#include "dsutil.hpp"
#include "dxutil.hpp"
#include "i18n.hpp"

// GLOBAL: TH07 0x004b9e68
Pbg4Archive g_UnusedPbg4ArchiveArray[20];

// FUNCTION: TH07 0x0045f6b0
Pbg4Archive::Pbg4Archive()
{
    this->entries = NULL;
    this->numOfEntries = 0;
    this->filename = NULL;
    this->fileAbstraction = NULL;
}

// FUNCTION: TH07 0x0045f6f0
Pbg4Archive::~Pbg4Archive()
{
    Release();
}

// FUNCTION: TH07 0x0045f710
bool Pbg4Archive::Load(const char *filename)
{
    Release();
    // STRING: TH07 0x00495100
    utils::DebugPrint("info : %s open arcfile\r\n", filename);

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
    // STRING: TH07 0x004950e8
    utils::DebugPrint("info : %s not found\r\n", filename);
    Release();
    return false;
}

// FUNCTION: TH07 0x0045f800
void Pbg4Archive::Release()
{
    // STRING: TH07 0x004950cc
    utils::DebugPrint("info : %s close arcfile\r\n", this->filename);
    if (this->filename)
    {
        GlobalFree(this->filename);
        this->filename = NULL;
    }
    SAFE_DELETE_ARRAY(this->entries);
    SAFE_DELETE(this->fileAbstraction);
    this->numOfEntries = 0;
}

#pragma var_order(entry, dstLen, dstBuf, srcBuf, dwBytes)
// FUNCTION: TH07 0x0045f960
u8 *Pbg4Archive::ReadDecompressEntry(const char *filename, u8 *buf)
{
    SIZE_T dstLen;
    Pbg4Entry *entry;
    SIZE_T dwBytes;
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
    srcBuf = (u8 *)GlobalAlloc(0, dwBytes);
    if (!srcBuf)
    {
        goto err;
    }

    if (!this->fileAbstraction->Seek(entry->dataOffset, g_SeekModes[FILE_BEGIN]))
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
        GlobalFree(srcBuf);
        srcBuf = NULL;
    }
    return dstBuf;
err:
    // STRING: TH07 0x004950b8
    utils::DebugPrint("info : %s error\r\n", this->filename);
    if (srcBuf)
    {
        GlobalFree(srcBuf);
        srcBuf = NULL;
    }
    return NULL;
}

// FUNCTION: TH07 0x0045fab0
u32 Pbg4Archive::GetEntrySize(const char *filename)
{
    Pbg4Entry *entry = FindEntry(filename);

    if (entry)
    {
        return entry->decompressedSize;
    }
    return 0;
}

// FUNCTION: TH07 0x0045fae0
Pbg4Entry *Pbg4Archive::FindEntry(const char *filename)
{
    if (!this->entries)
    {
        return NULL;
    }

    Pbg4Entry *entry = this->entries;
    for (i32 i = this->numOfEntries; i > 0; --i, ++entry)
    {
        if (stricmp(filename, entry->filename) == 0)
        {
            return entry;
        }
    }
    return NULL;
}

#pragma var_order(decompressedData, decompressedSize, magic, fileSize, \
                  headerSize, compressedData)
// FUNCTION: TH07 0x0045fb50
bool Pbg4Archive::OpenArchive(const char *path)
{
    u8 *compressedData;
    u32 headerSize;
    u32 fileSize;
    i32 magic;
    SIZE_T decompressedSize;
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

    this->fileAbstraction->Seek(headerSize, g_SeekModes[FILE_BEGIN]);
    compressedData = (u8 *)GlobalAlloc(0, fileSize);
    if (!compressedData)
    {
        goto err;
    }

    if (this->fileAbstraction->Read(compressedData, fileSize) == 0)
    {
        goto err;
    }

    decompressedData =
        Lzss::Decompress(compressedData, fileSize, NULL, decompressedSize);
    if (!decompressedData)
    {
        goto err;
    }

    this->entries =
        AllocEntries(decompressedData, this->numOfEntries, headerSize);
    if (!this->entries)
    {
        goto err;
    }

    if (compressedData)
    {
        GlobalFree(compressedData);
        compressedData = NULL;
    }
    if (decompressedData)
    {
        GlobalFree(decompressedData);
        decompressedData = NULL;
    }
    return true;
err:
    if (compressedData)
    {
        GlobalFree(compressedData);
        compressedData = NULL;
    }
    if (decompressedData)
    {
        GlobalFree(decompressedData);
        decompressedData = NULL;
    }
    SAFE_DELETE(this->fileAbstraction);
    // STRING: TH07 0x00495084
    utils::DebugPrint(TH_ERR_PBG4_ARC_OPEN_FAIL, path);
    while (false)
        ; // ZUN bloat: ??????
    return false;
}

#pragma var_order(entryData, i, entries)
// FUNCTION: TH07 0x0045fde0
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

        entries[i].dataOffset = *(u32 *)entryData;
        entryData += 4;

        entries[i].decompressedSize = *(u32 *)entryData;
        entryData += 4;

        entries[i].magicThing = *(u32 *)entryData;
        entryData += 4;
    }

    entries[count].dataOffset = dataOffset;
    entries[count].decompressedSize = 0;

    return entries;

err:
    SAFE_DELETE_ARRAY(entries);
    return NULL;
}

// FUNCTION: TH07 0x0045ffc0
char *Pbg4Archive::CopyFileName(const char *filename)
{
    char *filenameBuf;

    filenameBuf = (char *)GlobalAlloc(0, strlen(filename) + 1);
    if (filenameBuf)
    {
        strcpy(filenameBuf, filename);
    }
    return filenameBuf;
}
