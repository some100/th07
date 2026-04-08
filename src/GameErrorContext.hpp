#pragma once

#include "FileSystem.hpp"

struct GameErrorContext {
  char m_Buffer[8192];
  char *m_BufferEnd;
  bool m_ShowMessageBox;

  GameErrorContext() {
    m_BufferEnd = m_Buffer;
    m_Buffer[0] = '\0';
    m_ShowMessageBox = false;
    Log("東方動作記録 --------------------------------------------- \r\n");
  }

  const char *Fatal(const char *fmt, ...);
  const char *Log(const char *fmt, ...);

  // FUNCTION: TH07 0x00433e90
  void Flush() {
    if ((GameErrorContext *)this->m_BufferEnd != this) {
      this->Log(
          "---------------------------------------------------------- \r\n");
      if (this->m_ShowMessageBox != false) {
        MessageBoxA(NULL, this->m_Buffer, "log", 0x10);
      }
      FileSystem::WriteDataToFile("./log.txt", this, strlen(this->m_Buffer));
    }
  }
};

extern GameErrorContext g_GameErrorContext;
