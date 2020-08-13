#ifndef __DFS_SHARED__
#define __DFS_SHARED__

//Defined Boolean to make code more readable
typedef uint32 bool;
#define true 1
#define false 0

typedef struct dfs_superblock {
  // STUDENT: put superblock internals here
	bool valid; //Indicator if superblock is valid
	uint32 block_size; //file system block size
	uint32 num_blocks; //total number of file system blocks
	uint32 start_block_num_inodes_array; //starting file system block number for array of inodes
	uint32 size_inodes_array; //number of inodes in inodes array 
	uint32 start_block_num_fbv; //Starting file system block number for free block vector
	uint32 data_block_start; //Starting index of the datablocks
} dfs_superblock;

#define DFS_BLOCKSIZE 1024  // Must be an integer multiple of the disk blocksize

typedef struct dfs_block {
  char data[DFS_BLOCKSIZE];
} dfs_block;


#define DFS_FILENAME_SIZE 44 //File Name Size is: 96 = uint32 + uint32 + char * filename + uint32 * 10 + uint32 
#define DFS_VB_TRANSLATION_TABLE_SIZE 10
typedef struct dfs_inode {
  // STUDENT: put inode structure internals here
  // IMPORTANT: sizeof(dfs_inode) MUST return 128 in order to fit in enough //STUDENT: I'm guessing this is a typo, lab said 96 bytes not 128
  // inodes in the filesystem (and to make your life easier).  To do this, 
  // adjust the maximumm length of the filename until the size of the overall inode 
  // is 128 bytes.
 	bool inuse; //tells if inode is free or not
	uint32 size; //Size of the file this inode represents
	char filename[DFS_FILENAME_SIZE]; //Filename which is a string
	uint32 vb_translation_table[DFS_VB_TRANSLATION_TABLE_SIZE]; //Table of direct address translation for first 10 blocks
	uint32 block_num_indirect_index; //Index for table of indirect address translations
	
} dfs_inode;

#define DFS_MAX_FILESYSTEM_SIZE 0x1000000  // 16MB

#define DFS_INODE_MAX_NUM 192 //Lab requirement says 192 inodes
#define DFS_NUM_BLOCKS (DFS_MAX_FILESYSTEM_SIZE / DFS_BLOCKSIZE) //16384 blocks 
#define DFS_FBV_MAX_NUM_WORDS (DFS_NUM_BLOCKS / 32) //Each vector contains 32 bits -> 512


#define DFS_FAIL -1
#define DFS_SUCCESS 1



#endif
