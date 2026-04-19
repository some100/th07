#include "Lzss.hpp"

#include <windows.h>

// GLOBAL: TH07 0x0049fe30
Lzss::LzssNode g_LzssTree[0x2000 + 1];

// GLOBAL: TH07 0x004b7e40
u8 g_LzssDictionary[8196];

// FUNCTION: TH07 0x0045ead0
u8 *Lzss::Compress(u8 *src, i32 dstLen, i32 *outSize)
{
    u32 local_3c;
    u32 local_38;
    i32 local_34;
    i32 local_30;
    i32 local_28;
    u8 *local_24;
    u8 local_1d;
    i32 local_1c;
    i32 local_18;
    u32 local_14;
    u8 *local_10;
    u8 *local_c;
    u32 local_8;

    local_1d = 0x80;
    local_8 = 0;
    local_10 = (u8 *)GlobalAlloc(0, dstLen << 1);
    if (local_10 == NULL)
        return NULL;

    *outSize = 0;
    local_c = local_10;
    InitializeDictionary();
    local_38 = 1;
    local_24 = src;
    for (local_18 = 0; local_18 < 0x12; local_18 = local_18 + 1)
    {
        if (local_24 - src >= dstLen)
        {
            local_34 = -1;
        }
        else
        {
            local_34 = *local_24++;
        }
        if (local_34 == -1)
            break;
        g_LzssDictionary[local_18 + 1] = local_34;
    }
    local_30 = local_18;
    InitializeTree(1);
    local_28 = 0;
    local_14 = 0;
    while (local_30 > 0)
    {
        if (local_30 < local_28)
        {
            local_28 = local_30;
        }
        if (local_28 <= 2)
        {
            local_1c = 1;
            local_8 = local_8 | local_1d;
            local_1d = local_1d >> 1;
            if (local_1d == 0)
            {
                *local_10 = (u8)local_8;
                local_10 = local_10 + 1;
                local_8 = 0;
                local_1d = 0x80;
            }
            for (local_3c = 0x80; local_3c != 0; local_3c = local_3c >> 1)
            {
                if ((local_3c & g_LzssDictionary[local_38]) != 0)
                {
                    local_8 = local_8 | local_1d;
                }
                local_1d = local_1d >> 1;
                if (local_1d == 0)
                {
                    *local_10 = (u8)local_8;
                    local_10 = local_10 + 1;
                    local_8 = 0;
                    local_1d = 0x80;
                }
            }
        }
        else
        {
            local_1d = local_1d >> 1;
            if (local_1d == 0)
            {
                *local_10 = (u8)local_8;
                local_10 = local_10 + 1;
                local_8 = 0;
                local_1d = 0x80;
            }
            for (local_3c = 0x1000; local_3c != 0; local_3c = local_3c >> 1)
            {
                if ((local_3c & local_14) != 0)
                {
                    local_8 = local_8 | local_1d;
                }
                local_1d = local_1d >> 1;
                if (local_1d == 0)
                {
                    *local_10 = (u8)local_8;
                    local_10 = local_10 + 1;
                    local_8 = 0;
                    local_1d = 0x80;
                }
            }
            for (local_3c = 8; local_3c != 0; local_3c = local_3c >> 1)
            {
                if ((local_3c & local_28 - 3U) != 0)
                {
                    local_8 = local_8 | local_1d;
                }
                local_1d = local_1d >> 1;
                if (local_1d == 0)
                {
                    *local_10 = (u8)local_8;
                    local_10 = local_10 + 1;
                    local_8 = 0;
                    local_1d = 0x80;
                }
            }
            local_1c = local_28;
        }
        for (local_18 = 0; local_18 < local_1c; local_18 = local_18 + 1)
        {
            DeleteNode(local_38 + 0x12 & 0x1fff);
            if (local_24 - src >= dstLen)
            {
                local_34 = -1;
            }
            else
            {
                local_34 = *local_24++;
            }
            if (local_34 == -1)
            {
                local_30 = local_30 - 1;
            }
            else
            {
                g_LzssDictionary[local_38 + 0x12 & 0x1fff] = (u8)local_34;
            }
            local_38 = local_38 + 1 & 0x1fff;
            if (local_30 != 0)
            {
                local_28 = InsertNode(local_38, (i32 *)&local_14);
            }
        }
    }
    local_1d = local_1d >> 1;
    if (local_1d == 0)
    {
        *local_10 = (u8)local_8;
        local_10 = local_10 + 1;
        local_8 = 0;
        local_1d = 0x80;
    }
    for (local_3c = 0x1000; local_3c != 0; local_3c = local_3c >> 1)
    {
        local_1d = local_1d >> 1;
        if (local_1d == 0)
        {
            *local_10 = (u8)local_8;
            local_10 = local_10 + 1;
            local_8 = 0;
            local_1d = 0x80;
        }
    }
    *outSize = (i32)local_10 - (i32)local_c;
    return local_c;
}

// FUNCTION: TH07 0x0045ef00
u8 *Lzss::Decompress(u8 *src, i32 srcLen, u8 *dst, u32 dstLen)
{
    u8 bVar1;
    u32 uVar2;
    u8 bVar3;
    u32 local_38;
    u32 local_34;
    u32 local_20;
    u8 *local_1c;
    u8 local_15;
    i32 local_14;
    u8 *local_c;

    local_15 = 0x80;
    bVar1 = 0;
    if ((dst == NULL) && (dst = (u8 *)GlobalAlloc(0, dstLen), dst == NULL))
        return NULL;

    local_c = dst;
    local_38 = 1;
    local_1c = src;
    while (true)
    {
        while (true)
        {
            if (local_15 == 0x80)
            {
                bVar1 = *local_1c;
                if ((i32)local_1c - (i32)src < srcLen)
                {
                    local_1c = local_1c + 1;
                }
                else
                {
                    bVar1 = 0;
                }
            }
            bVar3 = bVar1 & local_15;
            local_15 = local_15 >> 1;
            if (local_15 == 0)
            {
                local_15 = 0x80;
            }
            if (bVar3 == 0)
                break;
            local_34 = 0x80;
            local_20 = 0;
            while (local_34 != 0)
            {
                if (local_15 == 0x80)
                {
                    bVar1 = *local_1c;
                    if ((i32)local_1c - (i32)src < srcLen)
                    {
                        local_1c = local_1c + 1;
                    }
                    else
                    {
                        bVar1 = 0;
                    }
                }
                if ((bVar1 & local_15) != 0)
                {
                    local_20 = local_20 | local_34;
                }
                local_34 = local_34 >> 1;
                local_15 = local_15 >> 1;
                if (local_15 == 0)
                {
                    local_15 = 0x80;
                }
            }
            *local_c = (u8)local_20;
            local_c = local_c + 1;
            g_LzssDictionary[local_38] = (u8)local_20;
            local_38 = local_38 + 1 & 0x1fff;
        }
        local_34 = 0x1000;
        local_20 = 0;
        while (uVar2 = local_20, local_34 != 0)
        {
            if (local_15 == 0x80)
            {
                bVar1 = *local_1c;
                if ((i32)local_1c - (i32)src < srcLen)
                {
                    local_1c = local_1c + 1;
                }
                else
                {
                    bVar1 = 0;
                }
            }
            if ((bVar1 & local_15) != 0)
            {
                local_20 = local_20 | local_34;
            }
            local_34 = local_34 >> 1;
            local_15 = local_15 >> 1;
            if (local_15 == 0)
            {
                local_15 = 0x80;
            }
        }
        if (local_20 == 0)
            break;
        local_34 = 8;
        local_20 = 0;
        while (local_34 != 0)
        {
            if (local_15 == 0x80)
            {
                bVar1 = *local_1c;
                if ((i32)local_1c - (i32)src < srcLen)
                {
                    local_1c = local_1c + 1;
                }
                else
                {
                    bVar1 = 0;
                }
            }
            if ((bVar1 & local_15) != 0)
            {
                local_20 = local_20 | local_34;
            }
            local_34 = local_34 >> 1;
            local_15 = local_15 >> 1;
            if (local_15 == 0)
            {
                local_15 = 0x80;
            }
        }
        for (local_14 = 0; local_14 <= (i32)(local_20 + 2);
             local_14 = local_14 + 1)
        {
            bVar3 = g_LzssDictionary[uVar2 + local_14 & 0x1fff];
            *local_c = bVar3;
            local_c = local_c + 1;
            g_LzssDictionary[local_38] = bVar3;
            local_38 = local_38 + 1 & 0x1fff;
        }
    }
    while (local_15 != 0x80)
    {
        if ((local_15 == 0x80) && ((i32)local_1c - (i32)src < srcLen))
        {
            local_1c = local_1c + 1;
        }
        local_15 = local_15 >> 1;
        if (local_15 == 0)
        {
            local_15 = 0x80;
        }
    }
    return dst;
}

// FUNCTION: TH07 0x0045f270
void Lzss::InitializeTree(i32 root)
{
    g_LzssTree[0x2000].rightChild = root;
    g_LzssTree[root].parent = 0x2000;
    g_LzssTree[root].rightChild = 0;
    g_LzssTree[root].leftChild = 0;
}

// FUNCTION: TH07 0x0045f2c0
void Lzss::InitializeDictionary()
{
    i32 i;

    for (i = 0; i < 0x2000; i++)
    {
        g_LzssDictionary[i] = 0;
    }
    for (i = 0; i < 0x2001; i++)
    {
        g_LzssTree[i].parent = 0;
        g_LzssTree[i].leftChild = 0;
        g_LzssTree[i].rightChild = 0;
    }
}

#pragma var_order(i, child, testNode, matchLength, delta)
// FUNCTION: TH07 0x0045f340
i32 Lzss::InsertNode(i32 node, i32 *matchPosition)
{
    i32 delta;
    i32 *child;
    i32 i;

    if (node == 0)
        return 0;

    i32 testNode = g_LzssTree[0x2000].rightChild;
    i32 matchLength = 0;
    for (;;)
    {
        for (i = 0; i < 0x12; ++i)
        {
            delta = (u32)g_LzssDictionary[node + i & 0x1fff] -
                    (u32)g_LzssDictionary[testNode + i & 0x1fff];
            if (delta != 0)
                break;
        }
        if (i >= matchLength)
        {
            matchLength = i;
            *matchPosition = testNode;
            if (matchLength >= 0x12)
            {
                ReplaceNode(testNode, node);
                return matchLength;
            }
        }
        if (delta >= 0)
        {
            child = &g_LzssTree[testNode].rightChild;
        }
        else
        {
            child = &g_LzssTree[testNode].leftChild;
        }
        if (*child == 0)
        {
            *child = node;
            g_LzssTree[node].parent = testNode;
            g_LzssTree[node].rightChild = 0;
            g_LzssTree[node].leftChild = 0;
            return matchLength;
        }
        testNode = *child;
    }
}

// FUNCTION: TH07 0x0045f460
void Lzss::DeleteNode(i32 node)
{
    if (g_LzssTree[node].parent == 0)
        return;

    if (g_LzssTree[node].rightChild == 0)
    {
        ContractNode(node, g_LzssTree[node].leftChild);
    }
    else if (g_LzssTree[node].leftChild == 0)
    {
        ContractNode(node, g_LzssTree[node].rightChild);
    }
    else
    {
        i32 iVar1 = FindMinNode(node);
        DeleteNode(iVar1);
        ReplaceNode(node, iVar1);
    }
}

// FUNCTION: TH07 0x0045f4f0
void Lzss::ContractNode(i32 firstNode, i32 secondNode)
{
    g_LzssTree[secondNode].parent = g_LzssTree[firstNode].parent;
    if (g_LzssTree[g_LzssTree[firstNode].parent].rightChild == firstNode)
    {
        g_LzssTree[g_LzssTree[firstNode].parent].rightChild = secondNode;
    }
    else
    {
        g_LzssTree[g_LzssTree[firstNode].parent].leftChild = secondNode;
    }
    g_LzssTree[firstNode].parent = 0;
}

// FUNCTION: TH07 0x0045f580
void Lzss::ReplaceNode(i32 oldNode, i32 newNode)
{
    i32 parent = g_LzssTree[oldNode].parent;

    if (g_LzssTree[parent].leftChild == oldNode)
    {
        g_LzssTree[parent].leftChild = newNode;
    }
    else
    {
        g_LzssTree[parent].rightChild = newNode;
    }

    g_LzssTree[newNode] = g_LzssTree[oldNode];
    g_LzssTree[g_LzssTree[newNode].leftChild].parent = newNode;
    g_LzssTree[g_LzssTree[newNode].rightChild].parent = newNode;
    g_LzssTree[oldNode].parent = 0;
}

// FUNCTION: TH07 0x0045f640
i32 Lzss::FindMinNode(i32 startNode)
{
    i32 node = g_LzssTree[startNode].leftChild;
    while (g_LzssTree[node].rightChild != 0)
        node = g_LzssTree[node].rightChild;
    return node;
}
