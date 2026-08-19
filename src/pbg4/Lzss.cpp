#include "Lzss.hpp"

#include <cstdlib>

#define LZSS_LOOKAHEAD_SIZE ((1 << LZSS_LENGTH_BITS) + 2)
#define LZSS_DICTSIZE_MASK (LZSS_DICTSIZE - 1)
#define LZSS_DICTPOS_MOD(pos, amount) ((pos + amount) & LZSS_DICTSIZE_MASK)

Lzss::LzssNode g_LzssTree[LZSS_DICTSIZE + 1];
u8 g_LzssDictionary[LZSS_DICTSIZE];

#define FALSE 0

#define ENC_NEXT_BIT()                                                                             \
    inBitMask >>= 1;                                                                               \
    if (inBitMask == 0)                                                                            \
    {                                                                                              \
        *dstCursor++ = curByte;                                                                    \
        checksum += curByte;                                                                       \
        curByte = 0;                                                                               \
        inBitMask = 0x80;                                                                          \
    }

#define ENC_WRITE_FLAG_BIT(bit)                                                                    \
    if (bit)                                                                                       \
    {                                                                                              \
        curByte |= inBitMask;                                                                      \
    }                                                                                              \
    ENC_NEXT_BIT();

#define ENC_WRITE_BITS(bitCount, condition)                                                        \
    bitfieldMask = 0x1 << (bitCount - 1);                                                          \
    while (bitfieldMask != 0)                                                                      \
    {                                                                                              \
        if (condition)                                                                             \
        {                                                                                          \
            curByte |= inBitMask;                                                                  \
        }                                                                                          \
        ENC_NEXT_BIT();                                                                            \
        bitfieldMask >>= 1;                                                                        \
    }

u8 *Lzss::Compress(u8 *src, i32 dstLen, i32 *srcLen)
{
    i32 i;
    i32 bytesToCopyToDict;
    i32 lookAheadBytes;
    i32 dictValue;
    u32 bitfieldMask;

    u8 inBitMask = 0x80;
    u32 curByte = 0;
    u32 checksum = 0;
    (void)checksum;

    u8 *dst = (u8 *)malloc(dstLen * 2);
    if (dst == NULL)
    {
        return NULL;
    }

    u8 *srcCursor = src;
    u8 *dstCursor = dst;
    *srcLen = 0;

    InitializeDictionary();

    u32 dictHead = 1;
    for (i = 0; i < LZSS_LOOKAHEAD_SIZE; i++)
    {
        if (srcCursor - src >= dstLen)
        {
            dictValue = -1;
        }
        else
        {
            dictValue = *srcCursor++;
        }

        if (dictValue == -1)
        {
            break;
        }

        g_LzssDictionary[dictHead + i] = dictValue;
    }

    lookAheadBytes = i;
    InitializeTree(dictHead);
    i32 matchLength = 0;
    i32 matchOffset = 0;

    while (lookAheadBytes > 0)
    {
        if (matchLength > lookAheadBytes)
        {
            matchLength = lookAheadBytes;
        }

        if (matchLength <= 2)
        {
            bytesToCopyToDict = 1;

            ENC_WRITE_FLAG_BIT(1);
            ENC_WRITE_BITS(8, (bitfieldMask & g_LzssDictionary[dictHead]) != 0);
        }
        else
        {
            ENC_WRITE_FLAG_BIT(0);
            ENC_WRITE_BITS(LZSS_OFFSET_BITS, (bitfieldMask & matchOffset) != 0);
            ENC_WRITE_BITS(LZSS_LENGTH_BITS, (bitfieldMask & (matchLength - 3)) != 0);

            bytesToCopyToDict = matchLength;
        }

        for (i = 0; i < bytesToCopyToDict; i++)
        {
            DeleteNode(LZSS_DICTPOS_MOD(dictHead, LZSS_LOOKAHEAD_SIZE));

            if (srcCursor - src >= dstLen)
            {
                dictValue = -1;
            }
            else
            {
                dictValue = *srcCursor++;
            }

            if (dictValue == -1)
            {
                lookAheadBytes--;
            }
            else
            {
                g_LzssDictionary[LZSS_DICTPOS_MOD(dictHead, LZSS_LOOKAHEAD_SIZE)] = dictValue;
            }

            dictHead = LZSS_DICTPOS_MOD(dictHead, 1);

            if (lookAheadBytes != 0)
            {
                matchLength = InsertNode(dictHead, &matchOffset);
            }
        }
    }

    ENC_WRITE_FLAG_BIT(0);
    ENC_WRITE_BITS(LZSS_OFFSET_BITS, FALSE);

    *srcLen = dstCursor - dst;
    return dst;
}

#define DEC_NEXT_BIT()                                                                             \
    inBitMask >>= 1;                                                                               \
    if (inBitMask == 0)                                                                            \
    {                                                                                              \
        inBitMask = 0x80;                                                                          \
    }

#define DEC_WRITE_BYTE(data)                                                                       \
    *dstCursor++ = data;                                                                           \
    g_LzssDictionary[dictHead] = data;                                                             \
    dictHead = LZSS_DICTPOS_MOD(dictHead, 1);

#define DEC_HANDLE_FETCH_NEW_BYTE()                                                                \
    if (inBitMask == 0x80)                                                                         \
    {                                                                                              \
        if (srcCursor - src >= size)                                                               \
        {                                                                                          \
            curByte = 0;                                                                           \
        }                                                                                          \
        else                                                                                       \
        {                                                                                          \
            curByte = *srcCursor;                                                                  \
            srcCursor++;                                                                           \
        }                                                                                          \
        checksum += curByte;                                                                       \
    }

#define DEC_READ_FLAG_BIT()                                                                        \
    DEC_HANDLE_FETCH_NEW_BYTE();                                                                   \
    inBits = curByte & inBitMask;                                                                  \
    DEC_NEXT_BIT();

#define DEC_READ_BITS(bitsCount)                                                                   \
    outBitMask = 0x01 << (bitsCount - 1);                                                          \
    inBits = 0;                                                                                    \
    while (outBitMask != 0)                                                                        \
    {                                                                                              \
        DEC_HANDLE_FETCH_NEW_BYTE();                                                               \
        if ((curByte & inBitMask) != 0)                                                            \
        {                                                                                          \
            inBits |= outBitMask;                                                                  \
        }                                                                                          \
                                                                                                   \
        outBitMask >>= 1;                                                                          \
        DEC_NEXT_BIT();                                                                            \
    }

u8 *Lzss::Decompress(u8 *src, i32 srcLen, u8 *dst, u32 decompressedSize)
{
    i32 i;
    u32 matchOffset;
    u32 inBits;
    i32 matchLength;
    u32 dictValue;
    u32 outBitMask;

    u8 inBitMask = 0x80;
    u32 curByte = 0;
    u32 checksum = 0;
    (void)checksum;
    i32 size = srcLen;

    if (dst == NULL)
    {
        dst = (u8 *)malloc(decompressedSize);
        if (dst == NULL)
        {
            return NULL;
        }
    }

    u8 *srcCursor = src;
    u8 *dstCursor = dst;
    u32 dictHead = 1;

    for (;;)
    {
        DEC_READ_FLAG_BIT();

        // Read literal byte from next 8 bits
        if (inBits != 0)
        {
            DEC_READ_BITS(8);
            DEC_WRITE_BYTE(inBits);
        }
        // Copy from dictionary, 13 bit offset, then 4 bit length
        else
        {
            DEC_READ_BITS(13);

            matchOffset = inBits;
            if (matchOffset == 0)
            {
                break;
            }

            DEC_READ_BITS(4);

            // Value encoded in 4 bit length is 3 less than the actual length
            matchLength = inBits + 2;
            for (i = 0; i <= matchLength; i++)
            {
                dictValue = g_LzssDictionary[LZSS_DICTPOS_MOD(matchOffset, i)];
                DEC_WRITE_BYTE(dictValue);
            }
        }
    }

    // Read any trailing bits in the data
    while (inBitMask != 0x80)
    {
        DEC_READ_FLAG_BIT();
    }

    return dst;
}

void Lzss::InitializeTree(i32 root)
{
    g_LzssTree[LZSS_DICTSIZE].rightChild = root;
    g_LzssTree[root].parent = LZSS_DICTSIZE;
    g_LzssTree[root].rightChild = 0;
    g_LzssTree[root].leftChild = 0;
}

void Lzss::InitializeDictionary()
{
    i32 i;

    for (i = 0; i < LZSS_DICTSIZE; i++)
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

i32 Lzss::InsertNode(i32 node, i32 *matchPosition)
{
    i32 delta;
    i32 *child;
    i32 i;

    if (node == 0)
    {
        return 0;
    }

    i32 testNode = g_LzssTree[LZSS_DICTSIZE].rightChild;
    i32 matchLength = 0;
    for (;;)
    {
        for (i = 0; i < 0x12; i++)
        {
            delta = (u32)g_LzssDictionary[(node + i) & 0x1fff] -
                    (u32)g_LzssDictionary[(testNode + i) & 0x1fff];
            if (delta != 0)
            {
                break;
            }
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

void Lzss::DeleteNode(i32 node)
{
    if (g_LzssTree[node].parent == 0)
    {
        return;
    }

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

i32 Lzss::FindMinNode(i32 startNode)
{
    i32 node = g_LzssTree[startNode].leftChild;
    while (g_LzssTree[node].rightChild != 0)
    {
        node = g_LzssTree[node].rightChild;
    }
    return node;
}
