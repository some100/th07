#pragma once

#include "Pbg4File.hpp"
#include "../inttypes.hpp"

struct Pbg4Entry {
  // FUNCTION: TH07 0x0045f680
  ~Pbg4Entry() {
    if (filename != NULL) {
      GlobalFree(filename);
      filename = NULL;
    }
  }

  char *filename;
  u32 dataOffset;
  u32 decompressedSize;
  u32 magicThing;
};

struct Pbg4Archive {
  Pbg4Archive();
  ~Pbg4Archive();
  Pbg4Entry *AllocEntries(void *param_1, i32 count, u32 dataOffset);
  char *CopyFileName(const char *filename);
  Pbg4Entry *FindEntry(const char *filename);
  u32 GetEntrySize(const char *param_1);
  bool Load(const char *param_1);
  bool OpenArchive(const char *path);
  u8 *ReadDecompressEntry(const char *filename, u8 *buf);
  void Release();

  Pbg4Entry *entries;
  i32 numOfEntries;
  char *filename;
  Pbg4File *fileAbstraction;
};

extern Pbg4Archive g_Pbg4Archive;
