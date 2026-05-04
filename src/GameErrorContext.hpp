#pragma once

#include "FileSystem.hpp"

struct GameErrorContext
{
    char m_Buffer[8192];
    char *m_BufferEnd;
    i8 m_ShowMessageBox;

    GameErrorContext()
    {
        m_BufferEnd = m_Buffer;
        m_Buffer[0] = '\0';
        m_ShowMessageBox = false;
        // STRING: TH07 0x00497dc8
        Log("“Œ•û“®ì‹L˜^ --------------------------------------------- \r\n");
    }

    const char *Fatal(const char *fmt, ...);
    const char *Log(const char *fmt, ...);

    // FUNCTION: TH07 0x00433e90
    void Flush()
    {
        if (this->m_BufferEnd != this->m_Buffer)
        {
            // STRING: TH07 0x00497c7c
            this->Log("---------------------------------------------------------- \r\n");
            if (this->m_ShowMessageBox)
            {
                // STRING: TH07 0x00497c78
                MessageBoxA(NULL, this->m_Buffer, "log", 0x10);
            }
            // STRING: TH07 0x00497c6c
            FileSystem::WriteDataToFile("./log.txt", this->m_Buffer,
                                        strlen(this->m_Buffer));
        }
    }
};
extern GameErrorContext g_GameErrorContext;
