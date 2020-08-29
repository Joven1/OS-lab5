#include "usertraps.h"
#include "misc.h"

#include "fdisk.h"

dfs_superblock sb;
dfs_inode inodes[DFS_INODE_MAX_NUM];
uint32 fbv[DFS_FBV_MAX_NUM_WORDS];

int diskblocksize = 0; // These are global in order to speed things up
int disksize = 0;      // (i.e. fewer traps to OS to get the same number)
int num_filesystem_blocks = 0;

int FdiskWriteBlock(uint32 blocknum, dfs_block *b); //You can use your own function. This function 
//calls disk_write_block() to write physical blocks to disk

void main (int argc, char *argv[])
{
	// STUDENT: put your code here. Follow the guidelines below. They are just the main steps. 
	// You need to think of the finer details. You can use bzero() to zero out bytes in memory
	int i,j; //Loop Control Variable 
	dfs_block block; 
	uint32 ptr; 

	Printf("\n\n======FDISK START======\n\n");	
	
//Initializations and argc check
	if(argc != 1)
	{
		Printf("ERROR: No Input\n\n");
		Exit();
	}


  // Need to invalidate filesystem before writing to it to make sure that the OS
  // doesn't wipe out what we do here with the old version in memory
  // You can use dfs_invalidate(); but it will be implemented in Problem 2. You can just do 
  // sb.valid = 0

	dfs_invalidate();

  //disksize = 
  //diskblocksize = 
  //num_filesystem_blocks = 

	//Check Size of dfs_inode
	if(sizeof(dfs_inode) != 96)
	{
		Printf("Error: DFS Inode is not 96 bytes!\n");
		Exit();
	}	
	else
	{
		Printf("Success: DFS Inode is 96 bytes!\n");
	}
 
 	Printf("The size of the dfs_inode is %d\n", sizeof(dfs_inode));
	disksize = disk_size();
	diskblocksize = disk_blocksize();
	num_filesystem_blocks = DFS_NUM_BLOCKS;

	
	//Print out the Sizes 
	//Initialize the Super Block
	sb.block_size = DFS_BLOCKSIZE;
	sb.num_blocks = DFS_NUM_BLOCKS;
	sb.start_block_num_inodes_array = FDISK_INODE_BLOCK_START;
	sb.size_inodes_array = FDISK_NUM_INODES;
	sb.start_block_num_fbv = FDISK_FBV_BLOCK_START;
	sb.data_block_start = FDISK_DATA_BLOCK_START;	
  
// Make sure the disk exists before doing anything else
	if(disk_create() == DISK_FAIL)
	{
		Printf("Error: Disk does not exist!\n");
		Exit();
	}	

  // Write all inodes as not in use and empty (all zeros)
	for(i = 0; i < 	sb.size_inodes_array; i++)
	{
		inodes[i].inuse = false;
		inodes[i].size = 0;
		inodes[i].filename[0] = '\0';
		for(j = 0; j < DFS_VB_TRANSLATION_TABLE_SIZE; j++)
		{
			inodes[i].vb_translation_table[j] = 0;
		}
		inodes[i].block_num_indirect_index = 0;
	}

  // Next, setup free block vector (fbv) and write free block vector to the disk
  	for(i = 0; i < sb.num_blocks/32; i++)
	{
		fbv[i] = 0; //0 Means it's free
	}
	
	fbv[0] = 0xFFFFF800; //Blocks 0-20 are inuse by the DFS 
	fbv[511] = 0xEEEEEEEE;	
	ptr = (char *) &(fbv[0]);

	
	//Write FBV into Disk Blocks 38 - 41 (DFS Blocks 19-20)
	for(i = sb.start_block_num_fbv; i < sb.data_block_start; i++)
	{
		bcopy(ptr, block.data, diskblocksize);
		FdiskWriteBlock(2*i, &block);
		ptr = ptr + diskblocksize; //Move Index Address by 512 Bytes 

		bcopy(ptr, block.data, diskblocksize);	
		FdiskWriteBlock(2*i + 1, &block);
		ptr = ptr + diskblocksize; //Move Index Addres by 512 Bytes
		
	}

  // Finally, setup superblock as valid filesystem and write superblock and boot record to disk: 
	sb.valid = true;
  
  // boot record is all zeros in the first physical block, and superblock structure goes into the second physical block
	bzero(block.data, DFS_BLOCKSIZE);
	bcopy((char *) &sb, block.data, sizeof(dfs_superblock));
	Printf("Running writing into the super block\n");	
	FdiskWriteBlock(1, &block);	


	ptr = (char *) &(inodes[0]);

	//Loop through and write in inodes 
	for(i = 0; i < (sizeof(inodes[0]) * sb.size_inodes_array)/sb.block_size; i++)
	{
		bcopy(ptr , block.data, diskblocksize);
		FdiskWriteBlock(2 * (sb.start_block_num_inodes_array  + i), &block);
		ptr += diskblocksize;

		bcopy(ptr, block.data, diskblocksize);
		FdiskWriteBlock(2 * (sb.start_block_num_inodes_array  + i) + 1, &block);
		ptr += diskblocksize;	
	}
	
	Printf("fdisk (%d): Formatted DFS disk for %d bytes.\n", getpid(), disksize);
}

int FdiskWriteBlock(uint32 blocknum, dfs_block *b) {
  // STUDENT: put your code here
  	if(disk_write_block(blocknum, b->data) == DISK_FAIL)
	{
		Printf("Failed to write block onto disk\n");
		Exit();
	}
	return DISK_SUCCESS;
}


