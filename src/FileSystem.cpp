#include "FileSystem.hpp"

#include <cstdio>

#include "GameErrorContext.hpp"
#include "Supervisor.hpp"
#include "pbg4/Pbg4Archive.hpp"

u32 g_LastFileSize;

u8 *FileSystem::OpenFile(const char *filepath, i32 isExternalResource)
{
    SDL_IOStream *file;
    u8 *buf;
    i64 fsize;
    const char *filename;

    if (!isExternalResource)
    {
        filename = strrchr(filepath, '\\');
        if (!filename)
        {
            filename = filepath;
        }
        else
        {
            filename++;
        }

        filename = strrchr(filename, '/');
        if (!filename)
        {
            filename = filepath;
        }
        else
        {
            filename++;
        }
        fsize = g_Pbg4Archive.GetEntrySize(filename);
        g_LastFileSize = fsize;
        if (fsize == 0)
        {
            g_GameErrorContext.Fatal("error : %s is not found in arcfile.\n", filename);
            return NULL;
        }
        if (fsize != 0)
        {
            Supervisor::DebugPrint("%s Decode ... \n", filename);
            buf = (u8 *)malloc(fsize);
            if (!buf)
            {
                return NULL;
            }

            g_Pbg4Archive.ReadDecompressEntry(filename, buf);
            return buf;
        }
    }
    Supervisor::DebugPrint("%s Load ... \n", filepath);
    file = SDL_IOFromFile(filepath, "rb");
    if (!file)
    {
        Supervisor::DebugPrint("error : %s is not found.\n", filepath);
        return NULL;
    }

    SDL_SeekIO(file, 0, SDL_IO_SEEK_END);
    fsize = SDL_TellIO(file);
    buf = (u8 *)malloc(fsize);
    if (!buf)
    {
        SDL_CloseIO(file);
        return NULL;
    }

    SDL_SeekIO(file, 0, SDL_IO_SEEK_SET);
    if (SDL_ReadIO(file, buf, fsize) != fsize)
    {
        SDL_CloseIO(file);
        return NULL;
    }
    g_LastFileSize = fsize;
    SDL_CloseIO(file);
    return buf;
}

i32 FileSystem::CheckFileExists(const char *file)
{
    SDL_IOStream *fp;

    fp = SDL_IOFromFile(FileSystem::GetPrefPath(file).c_str(), "rb");
    if (fp)
    {
        SDL_CloseIO(fp);
        return true;
    }
    return false;
}

i32 FileSystem::WriteDataToFile(const char *filename, const void *out, u32 bytesToWrite)
{
    SDL_IOStream *file;
    u32 bytesWritten;

    file = SDL_IOFromFile(FileSystem::GetPrefPath(filename).c_str(), "wb");
    if (!file)
    {
        Supervisor::DebugPrint("error : %s write error\n", filename);
        return -1;
    }

    bytesWritten = SDL_WriteIO(file, out, bytesToWrite);
    if (bytesToWrite != bytesWritten)
    {
        SDL_CloseIO(file);
        Supervisor::DebugPrint("error : %s write error\n", filename);
        return -2;
    }
    SDL_CloseIO(file);
    Supervisor::DebugPrint("%s write ...\n", filename);
    return 0;
}

std::string FileSystem::GetBasePath(const char *filename)
{
#if defined(TH_EXTERNAL_ASSETS)
    const char *path = nullptr;
#if defined(__ANDROID__)
    path = SDL_GetAndroidExternalStoragePath();
#elif defined(__APPLE__) && TARGET_OS_IPHONE
    path = SDL_GetUserFolder(SDL_FOLDER_DOCUMENTS);
#endif
    if (path)
    {
        return std::string(path) + '/' + filename;
    }
#endif
    const char *basePath = SDL_GetBasePath();
    if (basePath)
    {
        return std::string(basePath) + filename;
    }
    return std::string(filename);
}

std::string FileSystem::GetPrefPath(const char *filename)
{
#if defined(__EMSCRIPTEN__)
    return std::string("/savesth07/") + filename;
#elif defined(TH_EXTERNAL_ASSETS)
    return GetBasePath(filename);
#elif defined(__ANDROID__) || defined(__APPLE__)
    static char *prefPath = SDL_GetPrefPath("TeamShanghaiAlice", "th07");
    if (prefPath)
    {
        return std::string(prefPath) + filename;
    }
    return std::string(filename);
#else
    return std::string(filename);
#endif
}
