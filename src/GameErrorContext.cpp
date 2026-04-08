#include "GameErrorContext.hpp"

#include <stdio.h>

// GLOBAL: TH07 0x00624210
GameErrorContext g_GameErrorContext;

// FUNCTION: TH07 0x004315f0
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

// FUNCTION: TH07 0x00431730
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
