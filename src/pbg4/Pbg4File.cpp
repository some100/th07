#include "Pbg4File.hpp"

#include "../inttypes.hpp"

Pbg4File::Pbg4File() {
  this->handle = INVALID_HANDLE_VALUE;
  this->access = 0;
}

Pbg4File::~Pbg4File() { Close(); }

bool Pbg4File::Open(const char *path, const char *mode) {
  DWORD local_118;
  char local_114[264];
  i32 local_c;
  const char *local_8;

  local_c = 0;
  this->Close();
  for (local_8 = mode; *local_8 != '\0'; ++local_8) {
    if (*local_8 == 'r') {
      this->access = GENERIC_READ;
      local_118 = 3;
      break;
    }
    if (*local_8 == 'w') {
      DeleteFileA(path);
      this->access = GENERIC_WRITE;
      local_118 = 2;
      break;
    }
    if (*local_8 == 'a') {
      local_c = 1;
      this->access = GENERIC_WRITE;
      local_118 = 4;
      break;
    }
  }
  if (*local_8 == '\0') {
    return false;
  } else {
    GetFullPath(local_114, path);
    this->handle = CreateFileA(local_114, this->access, 1, NULL, local_118,
                               0x8000080, NULL);
    if (this->handle == INVALID_HANDLE_VALUE) {
      return false;
    } else {
      if (local_c != 0) {
        SetFilePointer(this->handle, 0, NULL, 2);
      }
      return true;
    }
  }
}

void Pbg4File::Close() {
  if (this->handle != INVALID_HANDLE_VALUE) {
    CloseHandle(this->handle);
    this->handle = INVALID_HANDLE_VALUE;
    this->access = 0;
  }
}

DWORD Pbg4File::Read(void *data, u32 len) {
  DWORD local_8;

  local_8 = 0;
  if (this->access == GENERIC_READ) {
    ReadFile(this->handle, data, len, &local_8, NULL);
  } else {
    local_8 = 0;
  }
  return local_8;
}

bool Pbg4File::Write(void *data, u32 len) {
  DWORD local_8;

  local_8 = 0;
  if (this->access == GENERIC_WRITE) {
    WriteFile(this->handle, data, len, &local_8, NULL);
    return len == local_8;
  } else {
    return false;
  }
}

DWORD Pbg4File::Tell() {
  if (this->handle == INVALID_HANDLE_VALUE) {
    return 0;
  } else {
    return SetFilePointer(this->handle, 0, NULL, 1);
  }
}

DWORD Pbg4File::GetSize() {
  if (this->handle == INVALID_HANDLE_VALUE) {
    return 0;
  } else {
    return GetFileSize(this->handle, NULL);
  }
}

bool Pbg4File::Seek(u32 offset, DWORD seekFrom) {
  bool bVar1 = this->handle != INVALID_HANDLE_VALUE;
  if (bVar1) {
    SetFilePointer(this->handle, (LONG)offset, NULL, seekFrom);
  }
  return bVar1;
}

HGLOBAL Pbg4File::ReadRemaining(u32 max) {
  bool bVar1;
  HGLOBAL hMem;
  DWORD DVar2;
  DWORD DVar3;

  if (this->access == GENERIC_READ) {
    DVar2 = this->GetSize();
    if (max < DVar2) {
      hMem = NULL;
    } else {
      hMem = GlobalAlloc(0x40, DVar2);
      if (hMem == NULL) {
        hMem = NULL;
      } else {
        DVar3 = this->Tell();
        bVar1 = this->Seek(DVar3, 0);
        if (bVar1) {
          DVar2 = this->Read(hMem, DVar2);
          if (DVar2 == 0) {
            if (hMem != NULL) {
              GlobalFree(hMem);
            }
            hMem = NULL;
          } else {
            this->Seek(DVar3, 0);
          }
        } else {
          hMem = NULL;
        }
      }
    }
  } else {
    hMem = NULL;
  }
  return hMem;
}

void Pbg4File::GetFullPath(char *out, const char *filename) {
  char *pcVar2;

  pcVar2 = strchr((char *)filename, ':');
  if (pcVar2 == NULL) {
    GetModuleFileNameA(NULL, out, 0x104);
    pcVar2 = strrchr(out, '\\');
    if (pcVar2 == NULL) {
      *out = '\0';
    }
    pcVar2[1] = '\0';
    strcat(out, filename);
  } else {
    strcpy(out, filename);
  }
}
