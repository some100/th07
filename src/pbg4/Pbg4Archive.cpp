#include "Pbg4Archive.hpp"

#include "../dsutil.hpp"
#include "../dxutil.hpp"
#include "Lzss.hpp"
#include "Pbg4File.hpp"

// GLOBAL: TH07 0x00626258
Pbg4Archive g_Pbg4Archive;

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
    DebugPrint("info : %s open arcfile\r\n", filename);

    this->fileAbstraction = new Pbg4File();

    if (this->fileAbstraction == NULL)
    {
        return false;
    }

    if (OpenArchive(filename))
    {
        this->filename = CopyFileName(filename);
        if (this->filename != NULL)
        {
            return true;
        }
    }
    // STRING: TH07 0x004950e8
    DebugPrint("info : %s not found\r\n", filename);
    Release();
    return false;
}

// FUNCTION: TH07 0x0045f800
void Pbg4Archive::Release()
{
    // STRING: TH07 0x004950cc
    DebugPrint("info : %s close arcfile\r\n", this->filename);
    if (this->filename != NULL)
    {
        GlobalFree(this->filename);
        this->filename = NULL;
    }
    SAFE_DELETE_ARRAY(this->entries);
    SAFE_DELETE(this->fileAbstraction);
    this->numOfEntries = 0;
}

// SYNTHETIC: TH07 0x0045f8d0
// Pbg4Entry::`vector deleting destructor'

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
    if (this->fileAbstraction == NULL)
    {
        return NULL;
    }

    entry = FindEntry(filename);
    if (entry == NULL)
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
    if (srcBuf == NULL)
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
    if (srcBuf != NULL)
    {
        GlobalFree(srcBuf);
        srcBuf = NULL;
    }
    return dstBuf;
err:
    // STRING: TH07 0x004950b8
    DebugPrint("info : %s error\r\n", this->filename);
    if (srcBuf != NULL)
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

    if (entry != NULL)
    {
        return entry->decompressedSize;
    }
    return 0;
}

// FUNCTION: TH07 0x0045fae0
Pbg4Entry *Pbg4Archive::FindEntry(const char *filename)
{
    if (this->entries == NULL)
    {
        return NULL;
    }

    Pbg4Entry *entry = this->entries;
    for (i32 i = this->numOfEntries; 0 < i; --i, ++entry)
    {
        if (stricmp(filename, entry->filename) == 0)
        {
            return entry;
        }
    }
    return NULL;
}

#pragma var_order(decompressedData, decompressedSize, magic, fileSize, \
                  headerSize, compressedData, ab1, ab2, ab3, ab4)
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
    if (this->fileAbstraction == NULL)
    {
        return false;
    }
    if (!this->fileAbstraction->Open(path, g_AccessModes[0]))
    {
        goto err;
    }

    Pbg4File *ab1 = this->fileAbstraction;
    if (ab1->Read(&magic, 4) == 0)
    {
        goto err;
    }
    if (magic != '4GBP')
    {
        goto err;
    }

    Pbg4File *ab2 = this->fileAbstraction;
    if (ab2->Read(&this->numOfEntries, 4) == 0)
    {
        goto err;
    }
    if (this->numOfEntries <= 0)
    {
        goto err;
    }

    fileSize = this->fileAbstraction->GetSize();

    Pbg4File *ab3 = this->fileAbstraction;
    if (ab3->Read(&headerSize, 4) == 0)
    {
        goto err;
    }
    if (headerSize >= fileSize)
    {
        goto err;
    }

    fileSize -= headerSize;

    Pbg4File *ab4 = this->fileAbstraction;
    if (ab4->Read(&decompressedSize, 4) == 0)
    {
        goto err;
    }

    this->fileAbstraction->Seek(headerSize, g_SeekModes[0]);
    compressedData = (u8 *)GlobalAlloc(0, fileSize);
    if (compressedData == NULL)
    {
        goto err;
    }

    if (this->fileAbstraction->Read(compressedData, fileSize) == 0)
    {
        goto err;
    }

    decompressedData =
        Lzss::Decompress(compressedData, fileSize, NULL, decompressedSize);
    if (decompressedData == NULL)
    {
        goto err;
    }

    this->entries =
        AllocEntries(decompressedData, this->numOfEntries, headerSize);
    if (this->entries == NULL)
    {
        goto err;
    }

    if (compressedData != NULL)
    {
        GlobalFree(compressedData);
        compressedData = NULL;
    }
    if (decompressedData != NULL)
    {
        GlobalFree(decompressedData);
        decompressedData = NULL;
    }
    return true;
err:
    if (compressedData != NULL)
    {
        GlobalFree(compressedData);
        compressedData = NULL;
    }
    if (decompressedData != NULL)
    {
        GlobalFree(decompressedData);
        decompressedData = NULL;
    }
    SAFE_DELETE(this->fileAbstraction);
    // STRING: TH07 0x00495084
    DebugPrint("ファイル %s のオープン中にエラーが発生しました\r\n", path);
    while (false)
        ; // ??????
    return false;
}

// FUNCTION: TH07 0x0045fde0
Pbg4Entry *Pbg4Archive::AllocEntries(LPVOID param_1, i32 count, u32 dataOffset)
{
    u8 *local_3c;
    Pbg4Entry *buffer = NULL;

    buffer = new Pbg4Entry[count + 1];

    if (buffer == NULL)
    {
        SAFE_DELETE_ARRAY(buffer);
        return NULL;
    }

    u8 *entryData = (u8 *)param_1;
    for (i32 i = 0; i < count; i++)
    {
        buffer[i].filename = CopyFileName((char *)entryData);
        local_3c = entryData;

        local_3c += strlen((char *)local_3c);
        buffer[i].dataOffset = *(u32 *)(local_3c + 1);
        buffer[i].decompressedSize = *(u32 *)(local_3c + 5);
        buffer[i].magicThing = *(u32 *)(local_3c + 9);
        entryData = local_3c + 0xd;
    }
    buffer[count].dataOffset = dataOffset;
    buffer[count].decompressedSize = 0;

    return buffer;
}

// FUNCTION: TH07 0x0045ffc0
char *Pbg4Archive::CopyFileName(const char *filename)
{
    char *pcVar2;

    pcVar2 = (char *)GlobalAlloc(0, strlen(filename) + 1);
    if (pcVar2 != NULL)
    {
        strcpy(pcVar2, filename);
    }
    return pcVar2;
}
