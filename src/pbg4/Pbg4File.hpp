#pragma once

#include <windows.h>

#include "../inttypes.hpp"

struct IPbg4File {
  IPbg4File() {}

  virtual bool Open(const char *path, const char *mode) = 0;
  virtual void Close() = 0;
  virtual DWORD Read(void *data, u32 len) = 0;
  virtual bool Write(void *data, u32 len) = 0;
  virtual DWORD Tell() = 0;
  virtual DWORD GetSize() = 0;
  virtual bool Seek(u32 offset, DWORD seekFrom) = 0;
  virtual ~IPbg4File() {}
};

struct Pbg4File : IPbg4File {
  Pbg4File();
  virtual ~Pbg4File();

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
