#include "FileSystem.hpp"

#include "GameErrorContext.hpp"
#include "pbg4/Pbg4Archive.hpp"
#include "dsutil.hpp"

// GLOBAL: TH07 0x004b9e64
u32 g_LastFileSize;

// FUNCTION: TH07 0x00431330
u8 *FileSystem::OpenFile(const char *filepath, i32 isExternalResource)

{
  const char *slashPos;
  u8 *buf;
  HANDLE hFile;
  size_t fsize;
  const char *filename;

  if (isExternalResource == 0) {
    slashPos = strrchr(filepath, '\\');
    filename = filepath;
    if (slashPos != NULL) {
      filename = slashPos + 1;
    }
    slashPos = strrchr(filename, '/');
    filename = filepath;
    if (slashPos != NULL) {
      filename = slashPos + 1;
    }
    g_LastFileSize = g_Pbg4Archive.GetEntrySize(filename);
    fsize = g_LastFileSize;
    if (g_LastFileSize == 0) {
      g_GameErrorContext.Fatal("error : %s is not found in arcfile.\r\n",
                               filename);
      return NULL;
    }
    if (g_LastFileSize != 0) {
      DebugPrint("%s Decode ... \r\n", filename);
      buf = (u8 *)malloc(fsize);
      if (buf == NULL) {
        return NULL;
      }
      g_Pbg4Archive.ReadDecompressEntry(filename, buf);
      return buf;
    }
  }
  DebugPrint("%s Load ... \r\n", filepath);
  hFile = CreateFileA(filepath, GENERIC_READ, 1, NULL, 3, 0x8000080, NULL);
  if (hFile == INVALID_HANDLE_VALUE) {
    DebugPrint("error : %s is not found.\r\n", filepath);
    buf = NULL;
  } else {
    fsize = GetFileSize(hFile, NULL);
    buf = (u8 *)malloc(fsize);
    if (buf == NULL) {
      CloseHandle(hFile);
      buf = NULL;
    } else {
      ReadFile(hFile, buf, fsize, (LPDWORD)&fsize, NULL);
      g_LastFileSize = fsize;
      CloseHandle(hFile);
    }
  }
  return buf;
}

// FUNCTION: TH07 0x004314f0
i32 FileSystem::CheckFileExists(const char *file)

{
  HANDLE hObject;

  hObject = CreateFileA(file, GENERIC_READ, 1, NULL, 3, 0x8000080, NULL);
  if (hObject != INVALID_HANDLE_VALUE) {
    CloseHandle(hObject);
  }
  return (u32)(hObject != INVALID_HANDLE_VALUE);
}

// FUNCTION: TH07 0x00431540
i32 FileSystem::WriteDataToFile(const char *filename, const void *out,
                                DWORD bytesToWrite)

{
  HANDLE hFile;
  DWORD bytesWritten;

  hFile = CreateFileA(filename, GENERIC_WRITE, 1, NULL, 2, 0x80, NULL);
  if (hFile == INVALID_HANDLE_VALUE) {
    DebugPrint("error : %s write error\r\n", filename);
    return -1;
  } else {
    WriteFile(hFile, out, bytesToWrite, &bytesWritten, NULL);
    if (bytesToWrite == bytesWritten) {
      CloseHandle(hFile);
      DebugPrint("%s write ...\r\n", filename);
      return 0;
    } else {
      CloseHandle(hFile);
      DebugPrint("error : %s write error\r\n", filename);
      return -2;
    }
  }
}
