#include "Pbg4Archive.hpp"

#include "../dsutil.hpp"
#include "../dxutil.hpp"
#include "Lzss.hpp"
#include "Pbg4File.hpp"

// GLOBAL: TH07 0x00626258
Pbg4Archive g_Pbg4Archive;

// would it really not have been simpler to just type the letter where its used
static const char *g_AccessModes[3] = {
    // STRING: TH07 0x00495244
    "r",
    // STRING: TH07 0x00495240
    "w",
    // STRING: TH07 0x0049523c
    "a"
};

// FUNCTION: TH07 0x0045f6b0
Pbg4Archive::Pbg4Archive() {
  this->entries = NULL;
  this->numOfEntries = 0;
  this->filename = NULL;
  this->fileAbstraction = NULL;
}

// FUNCTION: TH07 0x0045f6f0
Pbg4Archive::~Pbg4Archive() { Release(); }

// FUNCTION: TH07 0x0045f710
bool Pbg4Archive::Load(const char *filename)

{
  Release();
  // STRING: TH07 0x00495100
  DebugPrint("info : %s open arcfile\r\n", filename);

  this->fileAbstraction = new Pbg4File();

  if (this->fileAbstraction != NULL) {
    if (OpenArchive(filename)) {
      this->filename = CopyFileName(filename);
      if (this->filename != NULL) {
        return true;
      }
    }
    // STRING: TH07 0x004950e8
    DebugPrint("info : %s not found\r\n", filename);
    Release();
  }
  return false;
}

// FUNCTION: TH07 0x0045f800
void Pbg4Archive::Release()

{
  // STRING: TH07 0x004950cc
  DebugPrint("info : %s close arcfile\r\n", this->filename);
  if (this->filename != NULL) {
    GlobalFree(this->filename);
    this->filename = NULL;
  }
  SAFE_DELETE_ARRAY(this->entries);
  SAFE_DELETE(this->fileAbstraction);
  this->numOfEntries = 0;
}

// SYNTHETIC: TH07 0x0045f8d0
// Pbg4Entry::`vector deleting destructor'

// FUNCTION: TH07 0x0045f960
u8 *Pbg4Archive::ReadDecompressEntry(const char *filename, u8 *buf)

{
  SIZE_T dstLen;
  bool bVar1;
  Pbg4Entry *pPVar2;
  u32 uVar3;
  SIZE_T dwBytes;
  DWORD DVar4;
  u8 *pbVar5;
  u8 *local_14;

  local_14 = NULL;
  if (this->fileAbstraction != NULL) {
    pPVar2 = FindEntry(filename);
    if ((pPVar2 != NULL) &&
        (uVar3 = this->fileAbstraction->Open(this->filename, g_AccessModes[0]),
         (uVar3 & 0xff) != 0)) {
      dwBytes = pPVar2[1].dataOffset - pPVar2->dataOffset;
      dstLen = pPVar2->decompressedSize;
      local_14 = (u8 *)GlobalAlloc(0, dwBytes);
      if ((local_14 != NULL) &&
          ((bVar1 = this->fileAbstraction->Seek(pPVar2->dataOffset, 0),
            bVar1 && (DVar4 = this->fileAbstraction->Read(local_14, dwBytes),
                      DVar4 != 0)))) {
        pbVar5 = Lzss::Decompress(local_14, dwBytes, buf, dstLen);
        if (local_14 != NULL) {
          GlobalFree(local_14);
          return pbVar5;
        }
        return pbVar5;
      }
    }
    // STRING: TH07 0x004950b8
    DebugPrint("info : %s error\r\n", this->filename);
    if (local_14 != NULL) {
      GlobalFree(local_14);
    }
  }
  return NULL;
}

// FUNCTION: TH07 0x0045fab0
u32 Pbg4Archive::GetEntrySize(const char *filename)

{
  Pbg4Entry *entry = FindEntry(filename);

  if (entry == NULL) {
    return 0;
  } else {
    return entry->decompressedSize;
  }
}

// FUNCTION: TH07 0x0045fae0
Pbg4Entry *Pbg4Archive::FindEntry(const char *filename)

{
  if (this->entries != NULL) {
    Pbg4Entry *entry = this->entries;
    for (i32 i = this->numOfEntries; i > 0; --i) {
      if (stricmp(filename, entry->filename) == 0) {
        return entry;
      }
      entry += 1;
    }
  }
  return NULL;
}

// FUNCTION: TH07 0x0045fb50
bool Pbg4Archive::OpenArchive(const char *path)

{
  u32 uVar1;
  DWORD DVar2;
  Pbg4Entry *pPVar3;
  u8 *local_1c;
  u32 local_18;
  u32 local_14;
  i32 local_10;
  SIZE_T local_c;
  u8 *local_8;

  local_1c = NULL;
  local_8 = NULL;
  if (this->fileAbstraction != NULL) {
    uVar1 = this->fileAbstraction->Open(path, g_AccessModes[0]);
    if (((((uVar1 & 0xff) != 0) &&
          (DVar2 = this->fileAbstraction->Read(&local_10, 4), DVar2 != 0)) &&
         (local_10 == 0x34474250)) &&
        ((DVar2 = this->fileAbstraction->Read(&this->numOfEntries, 4),
          DVar2 != 0 && (0 < this->numOfEntries)))) {
      local_14 = this->fileAbstraction->GetSize();
      DVar2 = this->fileAbstraction->Read(&local_18, 4);
      if ((DVar2 != 0) && (local_18 < local_14)) {
        local_14 = local_14 - local_18;
        DVar2 = this->fileAbstraction->Read(&local_c, 4);
        if (DVar2 != 0) {
          this->fileAbstraction->Seek(local_18, 0);
          local_1c = (u8 *)GlobalAlloc(0, local_14);
          if (((local_1c != NULL) &&
               (DVar2 = this->fileAbstraction->Read(local_1c, local_14),
                DVar2 != 0)) &&
              (local_8 = Lzss::Decompress(local_1c, local_14, NULL, local_c),
               local_8 != NULL)) {
            pPVar3 = AllocEntries(local_8, this->numOfEntries, local_18);
            this->entries = pPVar3;
            if (this->entries != NULL) {
              if (local_1c != NULL) {
                GlobalFree(local_1c);
              }
              if (local_8 != NULL) {
                GlobalFree(local_8);
              }
              return true;
            }
          }
        }
      }
    }
    if (local_1c != NULL) {
      GlobalFree(local_1c);
    }
    if (local_8 != NULL) {
      GlobalFree(local_8);
      local_8 = NULL;
    }
    SAFE_DELETE(this->fileAbstraction);
    // STRING: TH07 0x00495084
    DebugPrint("ファイル %s のオープン中にエラーが発生しました\r\n", path);
  }
  return false;
}

// FUNCTION: TH07 0x0045fde0
Pbg4Entry *Pbg4Archive::AllocEntries(LPVOID param_1, i32 count, u32 dataOffset)

{
  u8 *local_3c;
  Pbg4Entry *buffer = NULL;

  buffer = new Pbg4Entry[count + 1]();

  if (buffer == NULL) {
    buffer = NULL;
  } else {
    u8 *entryData = (u8 *)param_1;
    for (i32 i = 0; i < count; i = i + 1) {
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
  }
  return buffer;
}

// FUNCTION: TH07 0x0045ffc0
char *Pbg4Archive::CopyFileName(const char *filename)

{
  char *pcVar2;

  pcVar2 = (char *)GlobalAlloc(0, strlen(filename) + 1);
  if (pcVar2 != NULL) {
    strcpy(pcVar2, filename);
  }
  return pcVar2;
}
