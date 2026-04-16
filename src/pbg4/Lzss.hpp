#pragma once

#include "../inttypes.hpp"

namespace Lzss
{
u8 *Compress(u8 *src, i32 dstLen, i32 *outSize);
u8 *Decompress(u8 *src, i32 srcLen, u8 *dst, u32 dstLen);
void ContractNode(i32 firstNode, i32 secondNode);
void DeleteNode(i32 node);
i32 FindMinNode(i32 startNode);
void InitializeDictionary();
void InitializeTree(i32 root);
i32 InsertNode(i32 node, i32 *matchPosition);
void ReplaceNode(i32 oldNode, i32 newNode);

struct LzssNode
{
    i32 parent;
    i32 leftChild;
    i32 rightChild;
};
} // namespace Lzss
