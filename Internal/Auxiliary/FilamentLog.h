// File: Source/Filament/Internal/Auxiliary/FilamentLog.h
#pragma once

/*====================================================================================================================================
                                                       FILAMENTLOG.H
====================================================================================================================================*/
// 🧩 Minimal logging utility for the Filament renderer.

#include <cstdio>
#include <cstdarg>

//------------------------------------------------------------------------------------------------------------------------
//                                                    LOGGING
//------------------------------------------------------------------------------------------------------------------------

namespace FilamentLog
{
    inline void info(const char* fmt, ...) noexcept
    {
        std::printf("[INFO]    ");
        va_list args;
        va_start(args, fmt);
        std::vprintf(fmt, args);
        va_end(args);
        std::printf("\n");
    }

    inline void warn(const char* fmt, ...) noexcept
    {
        std::printf("[WARN]    ");
        va_list args;
        va_start(args, fmt);
        std::vprintf(fmt, args);
        va_end(args);
        std::printf("\n");
    }

    inline void error(const char* fmt, ...) noexcept
    {
        std::fprintf(stderr, "[ERROR]   ");
        va_list args;
        va_start(args, fmt);
        std::vfprintf(stderr, fmt, args);
        va_end(args);
        std::fprintf(stderr, "\n");
    }
}
