//-----------------------------------------------------------------------------
// File: DXUtil.h
//
// Desc: Helper functions and typing shortcuts for DirectX programming.
//
// Copyright (c) 1997-2000 Microsoft Corporation. All rights reserved
//-----------------------------------------------------------------------------
#pragma once

#include "ZunMemory.hpp"

//-----------------------------------------------------------------------------
// Miscellaneous helper functions
//-----------------------------------------------------------------------------
#define SAFE_DELETE(p)  \
    {                   \
        if (p)          \
        {               \
            delete (p); \
            (p) = NULL; \
        }               \
    }
#define SAFE_DELETE_ARRAY(p) \
    {                        \
        if (p)               \
        {                    \
            delete[] (p);    \
            (p) = NULL;      \
        }                    \
    }
#define SAFE_FREE(p)            \
    {                           \
        if (p)                  \
        {                       \
            ZunMemory::Free(p); \
            (p) = NULL;         \
        }                       \
    }
#define SAFE_RELEASE(p)     \
    {                       \
        if (p)              \
        {                   \
            (p)->Release(); \
            (p) = NULL;     \
        }                   \
    }
