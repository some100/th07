#include "GameErrorContext.hpp"

#include <stdio.h>

#include "FileSystem.hpp"

GameErrorContext g_GameErrorContext;

const char *GameErrorContext::Log(const char *fmt, ...)

{
  char tmp[8196];
  va_list args;

  va_start(args, fmt);
  vsprintf(tmp, fmt, args);
  if (this->m_BufferEnd + strlen(tmp) < this->m_Buffer + 0x1fff) {
    strcpy(this->m_BufferEnd, tmp);

    this->m_BufferEnd += strlen(tmp);
    *this->m_BufferEnd = '\0';
  }
  return fmt;
}

const char *GameErrorContext::Fatal(const char *fmt, ...)

{
  char tmp[516];
  va_list args;

  va_start(args, fmt);
  vsprintf(tmp, fmt, args);
  if (this->m_BufferEnd + strlen(tmp) < this->m_Buffer + 0x1fff) {
    strcpy(this->m_BufferEnd, tmp);
    this->m_BufferEnd = this->m_BufferEnd + strlen(tmp);
    *this->m_BufferEnd = '\0';
  }
  this->m_ShowMessageBox = true;
  return fmt;
}
