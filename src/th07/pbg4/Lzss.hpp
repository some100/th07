#pragma once

#include "inttypes.hpp"

#define LZSS_OFFSET_BITS 13
#define LZSS_LENGTH_BITS 4
#define LZSS_DICTSIZE (1 << LZSS_OFFSET_BITS)

namespace Lzss
{
u8 *Compress(u8 *src, i32 dstLen, i32 *outSize);
u8 *Decompress(u8 *src, i32 srcLen, u8 *dst, u32 dstLen);

void InitTree(i32 root);
void InitializeDictionary();
i32 AddString(i32 node, i32 *matchPosition);
void DeleteString(i32 node);
void ContractNode(i32 firstNode, i32 secondNode);
void ReplaceNode(i32 oldNode, i32 newNode);
i32 FindNextNode(i32 startNode);

struct LzssNode
{
    i32 parent;
    i32 leftChild;
    i32 rightChild;
};
} // namespace Lzss
