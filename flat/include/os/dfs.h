#ifndef __DFS_H__
#define __DFS_H__

#include "dfs_shared.h"

void DfsModuleInit();
int DfsOpenFileSystem();
void DfsInvalidate();
int DfsCloseFileSystem();
uint32 DfsAllocateBlock();
int DfsFreeBlock(uint32 blocknum);
int DfsReadBlock(uint32 blocknum, dfs_block * b);
int DfsWriteBlock(uint32 blocknum, dfs_block * b);

#endif
