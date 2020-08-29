#include "ostraps.h"
#include "dlxos.h"
#include "traps.h"
#include "queue.h"
#include "disk.h"
#include "dfs.h"
#include "synch.h"

static dfs_inode inodes[DFS_INODE_MAX_NUM]; // all inodes
static dfs_superblock sb; // superblock
static uint32 fbv[DFS_FBV_MAX_NUM_WORDS]; // Free block vector

static bool fs_open = false;

static uint32 negativeone = 0xFFFFFFFF;
static inline uint32 invert(uint32 n) { return n ^ negativeone; }

// You have already been told about the most likely places where you should use locks. You may use 
// additional locks if it is really necessary.
static lock_t fbv_lock;
static lock_t inode_lock;

// STUDENT: put your file system level functions below.
// Some skeletons are provided. You can implement additional functions.
uint32 DfsCheckSystem()
{
	if(sb.valid == false) //if super block isn't valid
	{
		return DFS_FAIL;
	}
	else if(fs_open == false) //if filesystem isn't open 
	{
		return DFS_FAIL;
	}
	
	return DFS_SUCCESS;
}

void PrintSuperBlockStatus()
{
	printf("Super Block Status\n");
	printf("Valid: %d\n", sb.valid);
	printf("Block Size: %d\n", sb.block_size);
	printf("Total Blocks: %d\n", sb.num_blocks);
	printf("Inode Start: %d\n", sb.start_block_num_inodes_array);
	printf("Size Inode Array: %d\n", sb.size_inodes_array);
	printf("Start Block Number FBV: %d\n", sb.start_block_num_fbv);
	printf("Data Block Start: %d\n", sb.data_block_start);
}

void PrintFBV()
{
	uint32 i;
	printf("Free Block Vector: \n");
	for(i = 0; i < DFS_FBV_MAX_NUM_WORDS; i++)
	{
		printf("%x ", fbv[i]);
	}
	printf("FREE BLOCK VECTOR SIZE IS %d\n", DFS_FBV_MAX_NUM_WORDS);
}

void PrintInodeStatus(uint32 i)
{
	printf("Inode Status Inode %d\n", i);
	printf("Inuse: %d \n", inodes[i].inuse);
	printf("Size: %d \n", inodes[i].size);
	printf("Filename: %s \n", inodes[i].filename);
}

///////////////////////////////////////////////////////////////////
// Non-inode functions first
///////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------
// DfsModuleInit is called at boot time to initialize things and
// open the file system for use.
//-----------------------------------------------------------------

void DfsModuleInit() {
// You essentially set the file system as invalid and then open 
// using DfsOpenFileSystem().
	DfsInvalidate(); //Set file system as invalid
	DfsOpenFileSystem();

	//Allocate Locks, if I did it by the static variables, this might not
	//run properly on multiple machines
	fbv_lock = LockCreate();
	inode_lock = LockCreate();
	
	return;
}

//-----------------------------------------------------------------
// DfsInavlidate marks the current version of the filesystem in
// memory as invalid.  This is really only useful when formatting
// the disk, to prevent the current memory version from overwriting
// what you already have on the disk when the OS exits.
//-----------------------------------------------------------------

void DfsInvalidate() {
// This is just a one-line function which sets the valid bit of the 
// superblock to n;
	//super block is no longer valid
	sb.valid = false;
	return;
}

//-------------------------------------------------------------------
// DfsOpenFileSystem loads the file system metadata from the disk
// into memory.  Returns DFS_SUCCESS on success, and DFS_FAIL on 
// failure.
//-------------------------------------------------------------------

int DfsOpenFileSystem() 
{

	uint32 i,j;
	disk_block buffer; //Buffer to read in data from disk
	int ret_val;
	char * ptr; //pointer to memory to copy into
	uint32 byte_ratio;
	uint32 inodes_blocks; //number of blocks the inodes occupy
	uint32 diskblocksize = DiskBytesPerBlock();

	printf("\n\nRunning DFS Open File System. \n\n\n");

  // Basic steps:
  // Check that filesystem is not already open
	if(fs_open == true) //Check whether or not file system was open before initialization
	{
		return DFS_FAIL;
	}

  //Set the open flag
	fs_open = true;

  // Read superblock from disk.  Note this is using the disk read rather 
  // than the DFS read function because the DFS read requires a valid 
  // filesystem in memory already, and the filesystem cannot be valid 
  // until we read the superblock. Also, we don't know the block size 
  // until we read the superblock, either.
	ret_val = DiskReadBlock(1, &buffer);
	if(ret_val == DISK_FAIL)
	{
		printf("Error: Unable to read from Super Block in Disk\n");
		return DFS_FAIL;
	}

  // Copy the data from the block we just read into the superblock in memory
	bcopy(buffer.data, (char *) (&sb), sizeof(dfs_superblock));
	
  // All other blocks are sized by virtual block size:
  // Read inodes
  //Since we are reading via Disk, there is a difference between the physical block sizes and file system block sizes
  //ex. if a file system uses 1024 bytes and disk uses 512 bytes -> block num 5 = blocks 10 + 11
	byte_ratio = sb.block_size / DiskBytesPerBlock();

	ptr = (char *) &(inodes[0]); //Obtain Address of the inode array

	inodes_blocks = (sizeof(inodes[0]) * sb.size_inodes_array)/sb.block_size;
	
	//Loop from starting inode to starting block number for free block vector
	for( i = 0 ; i < inodes_blocks; i++)
	{
		for(j = 0; j < byte_ratio; j++) //Each DFS block could span multiple physical Blocks
		{
			ret_val = DiskReadBlock(byte_ratio * (sb.start_block_num_inodes_array + i) + j, &buffer);
			if(ret_val != diskblocksize)
			{
				printf("Error: Disk Read Fail full Bytes\n");
				return DFS_FAIL;
			}
			bcopy(buffer.data, ptr, diskblocksize); //Copy buffer into ptr 
			ptr = ptr + diskblocksize;
		}
	}

  // Read free block vector
  	ptr = (char *) &(fbv[0]);

	//Loop from starting fbv block num to data block num start (Disk Blocks 38-41) (DFS Blocks 19-20)
	for( i = sb.start_block_num_fbv; i < sb.data_block_start; i++)
	{
		for(j = 0; j < byte_ratio; j++) //Each DFS block could span multiple physical blocks
		{
			ret_val = DiskReadBlock(byte_ratio * i + j, &buffer);
			if(ret_val != diskblocksize)
			{
				printf("Error: Disk Read Fail full Bytes\n");
				return DFS_FAIL;
			}	
			bcopy(buffer.data, ptr, diskblocksize); //Copy buffer into ptr 
			ptr = ptr + diskblocksize; 
		}
	}	

  // Change superblock to be invalid, write back to disk, then change 
  // it back to be valid in memory
	DfsInvalidate();
		
	bzero(buffer.data, diskblocksize); //zero out the buffer 
	bcopy((char *) &sb, buffer.data, sizeof(sb)); //Copy data from super block into buffer

	ret_val = DiskWriteBlock(1, &buffer); //Write back to disk	
	if( ret_val == DISK_FAIL)
	{
		printf("Error: Unable to write from Super Block in Disk\n");
		return DFS_FAIL;
	}

	//Change to be valid in memory
	sb.valid = true;
	return DFS_SUCCESS;

}


//-------------------------------------------------------------------
// DfsCloseFileSystem writes the current memory version of the
// filesystem metadata to the disk, and invalidates the memory's 
// version.
//-------------------------------------------------------------------

int DfsCloseFileSystem() 
{
	uint32 i,j;
	disk_block buffer; //Buffer to read in data from disk
	int ret_val;
	char * ptr; //pointer to memory to copy into
	uint32 byte_ratio;
	uint32 inodes_blocks; //number of blocks the inodes occupy
	uint32 diskblocksize = DiskBytesPerBlock();

	printf("\n\nRunning DFS Close File System. \n\n\n");

	if(DfsCheckSystem() == DFS_FAIL)
	{
		printf("Error: Filesystem Invalid to close.\n");
		return DFS_FAIL;
	}
	
  //Since we are reading via Disk, there is a difference between the physical block sizes and file system block sizes
  //ex. if a file system uses 1024 bytes and disk uses 512 bytes -> block num 5 = blocks 10 + 11
	byte_ratio = sb.block_size / DiskBytesPerBlock();

	ptr = (char *) &(inodes[0]); //Obtain Address of the inode array

	inodes_blocks = (sizeof(inodes[0]) * sb.size_inodes_array)/sb.block_size;
	
	//Write in the inodes to memory
	for( i = 0 ; i < inodes_blocks; i++)
	{
		for(j = 0; j < byte_ratio; j++) //Each DFS block could span multiple physical Blocks
		{	
			bzero(buffer.data, diskblocksize);
			bcopy(ptr, buffer.data, diskblocksize); //Copy the ptr into buffer 
			ret_val = DiskWriteBlock(byte_ratio * (sb.start_block_num_inodes_array + i) + j, &buffer); //Write into Disk
			if(ret_val != diskblocksize)
			{
				printf("Error: Disk Read Fail full Bytes\n");
				return DFS_FAIL;
			}
			ptr = ptr + diskblocksize;
		}
	}

  // Write in the free block vector
  	ptr = (char *) &(fbv[0]);

	//Loop from starting fbv block num to data block num start (Disk Blocks 38-41) (DFS Blocks 19-20)
	for( i = sb.start_block_num_fbv; i < sb.data_block_start; i++)
	{
		for(j = 0; j < byte_ratio; j++) //Each DFS block could span multiple physical blocks
		{
			bzero(buffer.data, diskblocksize);
			bcopy(ptr, buffer.data, diskblocksize); //Copy buffer into ptr 
			ret_val = DiskWriteBlock(byte_ratio * i + j, &buffer);
			if(ret_val != diskblocksize)
			{
				printf("Error: Disk Read Fail full Bytes\n");
				return DFS_FAIL;
			}	
			ptr = ptr + diskblocksize; 
		}
	}	


	//Write Super Block into Disk Last to make sure other actions succeed
	bzero(buffer.data, diskblocksize); //zero out the buffer 
	bcopy((char *) &sb, buffer.data, sizeof(sb)); //Copy data from super block into buffer
	
	ret_val = DiskWriteBlock(1, &buffer); //Write back to disk	
	if( ret_val == DISK_FAIL)
	{
		printf("Error: Unable to write from Super Block in Disk\n");
		return DFS_FAIL;
	}

	//Invalidate Memory's Version
	DfsInvalidate();
	fs_open = false;
	return DFS_SUCCESS;


}

bool isKthBitSet(uint32 n, uint32 k)
{
	if((n >> (k - 1)) & 1)
	{
		return true;
	}
	else
	{
		return false;
	}
}

uint32 find_Open_Bit(uint32 n)
{
	uint32 i;

	//Loop through the MSB to LSB
	for(i = 32; i-- > 0 ;)
	{
		if(n & (1 << i))
		{

		}
		else
		{
			return 31 - i;
		}
	}
	return 0;
}

uint32 find_Open_Block(uint32 index, uint32 block_value)
{
	return 32 * index + find_Open_Bit(block_value);
} 

//-----------------------------------------------------------------
// DfsAllocateBlock allocates a DFS block for use. Remember to use 
// locks where necessary.
//-----------------------------------------------------------------

uint32 DfsAllocateBlock() 
{

	uint32 i; //Loop Control Variables
	uint32 block_position;

	printf("Allocating Block \n");
	// Check that file system has been validly loaded into memory
	if( DfsCheckSystem() == DFS_FAIL)
	{
		return DFS_FAIL;
	}
	
	// Find the first free block using the free block vector (FBV), mark it in use
	if(LockHandleAcquire(fbv_lock) != SYNC_SUCCESS)
	{
		printf("Error: FBV Lock Unavailable\n");
		return DFS_FAIL;
	}

	for(i = 0; i < DFS_FBV_MAX_NUM_WORDS; i++)
	{
		//Entry with "0" (not in use) has been found
		if(fbv[i] != negativeone)
		{
			block_position = find_Open_Block(i, fbv[i]); //Find the exact block position
			fbv[i] = fbv[i] ^ (0x1 << (31 - block_position)); //Set the Free Block Vector by left shifting a mask
			
			if(LockHandleRelease(fbv_lock) != SYNC_SUCCESS)		
			{
				printf("Error FBV Lock Release Failed\n");
				return DFS_FAIL;
			}

			//return handle to block
			return block_position;
		
		}
	}
	// Return handle to block

	if(LockHandleRelease(fbv_lock) != SYNC_SUCCESS)
	{
		printf("Error FBV Lock Release Failed\n");
		return DFS_FAIL;
	}

	//If we made it through the for loop, that means no data blocks are available
	printf("Error: All Blocks are Occupied\n");
	return DFS_FAIL; 
}


//-----------------------------------------------------------------
// DfsFreeBlock deallocates a DFS block.
//-----------------------------------------------------------------

//Helper
//Return Whether or Not a Block is Allocated
bool block_allocated(uint32 blocknum)
{
	uint32 fbv_index;
	uint32 index_bit_position;
	uint32 mask;

	fbv_index = blocknum/32;
	index_bit_position = blocknum % 32;
	mask = 0x1 << (31 - index_bit_position);

	if(fbv[fbv_index] & mask)
	{
		return true;
	}
	else
	{
		return false;
	}
}
int DfsFreeBlock(uint32 blocknum) 
{
	
	uint32 fbv_index;
	uint32 index_bit_position;
	uint32 mask;
	
	if(LockHandleAcquire(fbv_lock) != SYNC_SUCCESS)
	{
		printf("Error: FBV Lock Unavailable\n");
		return DFS_FAIL;
	}

	//Get Index and Bit Position of Block Number	
	fbv_index = blocknum/32; 
	index_bit_position = blocknum % 32;

	mask = 0x1 << (31 - index_bit_position);
		
	//See if Bit is Set Already, if it is, return an error
	if(block_allocated(blocknum))
	{
		fbv[fbv_index] = fbv[fbv_index] ^ mask; //UnSet bit	

		if(LockHandleRelease(fbv_lock) != SYNC_SUCCESS)
		{
			printf("Error FBV Lock Release Failed\n");
			return DFS_FAIL;
		}
		
		return DFS_SUCCESS;
	}
	else
	{
		printf("Error: Block has already been freed!\n");
		if(LockHandleRelease(fbv_lock) != SYNC_SUCCESS)
		{
			printf("Error FBV Lock Release Failed\n");
			return DFS_FAIL;
		}
		return DFS_FAIL;
	}
}


//-----------------------------------------------------------------
// DfsReadBlock reads an allocated DFS block from the disk
// (which could span multiple physical disk blocks).  The block
// must be allocated in order to read from it.  Returns DFS_FAIL
// on failure, and the number of bytes read on success.  
//-----------------------------------------------------------------

int DfsReadBlock(uint32 blocknum, dfs_block *b) {

	uint32 i;
	disk_block db;
	uint32 diskblocksize = DiskBytesPerBlock();
	uint32 bytes_written = 0;
	uint32 byte_ratio = sb.block_size/diskblocksize;
	
	//First Check if File System is Valid
	if(DfsCheckSystem() == DFS_FAIL)
	{
		printf("Error: File System Invalid\n");
		return DFS_FAIL;
	}

	//Check if the block has already been allocated
	if(!block_allocated(blocknum))
	{
		printf("Read Error: Block has not been allocated!\n");
		return DFS_FAIL;
	}	

	for( i = 0; i < byte_ratio; i++)
	{
		//Zero out data
		bzero(db.data, diskblocksize);

		//Read in from disk
		if(DiskReadBlock(blocknum * byte_ratio + i, &db) == DISK_FAIL)
		{
			printf("Error: Could not write to disk!\n");
			return DFS_FAIL;
		}	

		//Copy into the dfs block
		bcopy(db.data, &(b->data[i * diskblocksize]), diskblocksize);	
		bytes_written = bytes_written + diskblocksize;	
	}

	return bytes_written;
}


//-----------------------------------------------------------------
// DfsWriteBlock writes to an allocated DFS block on the disk
// (which could span multiple physical disk blocks).  The block
// must be allocated in order to write to it.  Returns DFS_FAIL
// on failure, and the number of bytes written on success.  
//-----------------------------------------------------------------

int DfsWriteBlock(uint32 blocknum, dfs_block *b)
{
	uint32 i;
	disk_block db;
	uint32 diskblocksize = DiskBytesPerBlock();
	uint32 bytes_written = 0;
	uint32 byte_ratio = sb.block_size/diskblocksize;
	
	//First Check if File System is Valid
	if(DfsCheckSystem() == DFS_FAIL)
	{
		printf("Error: File System Invalid\n");
		return DFS_FAIL;
	}

	//Check if the block has already been allocated
	if(!block_allocated(blocknum))
	{
		printf("Write Error: Block has not been allocated!\n");
		return DFS_FAIL;
	}	

	//Since our file system block could span multiple blocks, so use the byte ratio to loop through
	for( i = 0; i < byte_ratio; i++)
	{
		//Zero out the data block before copying to a disk block
		bzero(db.data, diskblocksize);

		//Copy data from dfs block into a disk sized block
		bcopy(&(b->data[i * diskblocksize]), db.data, diskblocksize);
	
		//Write to Disk
		if(DiskWriteBlock(blocknum * byte_ratio + i, &db) == DISK_FAIL)
		{
			printf("Error: Could not write to disk!\n");
			return DFS_FAIL;
		}	
		
		bytes_written = bytes_written + diskblocksize;	
	}

	return bytes_written;
}

////////////////////////////////////////////////////////////////////////////////
// Inode-based functions
////////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------
// DfsInodeFilenameExists looks through all the inuse inodes for 
// the given filename. If the filename is found, return the handle 
// of the inode. If it is not found, return DFS_FAIL.
//-----------------------------------------------------------------

uint32 DfsInodeFilenameExists(char *filename) {
	
	uint32 i;

	//If the filename size is too long, then it shouldn't exist
	if(dstrlen(filename) > DFS_FILENAME_SIZE)
	{
		printf("Error: Filename is too large, make it less than DFS_FILENAME_SIZE: %d\n", DFS_FILENAME_SIZE);
		return DFS_FAIL;
	}

	//Loop through list of inodes
	for(i = 0; i < DFS_INODE_MAX_NUM; i++)
	{
		if(inodes[i].inuse == true) //If the inode is inuse
		{
			//We compare for strlen == filename, since there are cases where we compare "hell" to "hello
			if(dstrncmp(filename, inodes[i].filename, dstrlen(filename)) == 0)
			{
				return i; //return handle
			}	
		}	
	}

	//No filename found, failure
	return DFS_FAIL;
}


//-----------------------------------------------------------------
// DfsInodeOpen: search the list of all inuse inodes for the 
// specified filename. If the filename exists, return the handle 
// of the inode. If it does not, allocate a new inode for this 
// filename and return its handle. Return DFS_FAIL on failure. 
// Remember to use locks whenever you allocate a new inode.
//-----------------------------------------------------------------

uint32 DfsInodeOpen(char *filename) {
	
	uint32 inode_handle; //handle for inode
	uint32 i; //loop control variable	
	uint32 j; //secondary loop control variable

	//Try to see if filename exists
	inode_handle = DfsInodeFilenameExists(filename);
 
	
	if( inode_handle  != DFS_FAIL )
	{
		return inode_handle;
	}

	//If the filename size is too long, then it shouldn't exist
	if(dstrlen(filename) > DFS_FILENAME_SIZE)
	{
		printf("Error: Filename is too large, make it less than DFS_FILENAME_SIZE: %d\n", DFS_FILENAME_SIZE);
		return DFS_FAIL;
	}
	
	//Acquire Lock to secure inode
	if(LockHandleAcquire(inode_lock) != SYNC_SUCCESS)
	{
		printf("Error: Inode Lock Unavailable\n");
		return DFS_FAIL;
	}

	//Loop through inodes
	for(i = 0; i < DFS_INODE_MAX_NUM; i++)
	{
		if(inodes[i].inuse == false) //Find an available inode
		{
			//Allocate Inode
			inode_handle = i;
			inodes[i].inuse = true;
			inodes[i].size = 0;		
			dstrncpy(inodes[i].filename, filename, DFS_FILENAME_SIZE); //Copy filename into inode
			
			//Zero out the address table
			for(j = 0; j < DFS_VB_TRANSLATION_TABLE_SIZE; j++)
			{
				inodes[i].vb_translation_table[j] = 0;
			}
			inodes[i].block_num_indirect_index = 0;
			break;
		}	
	} 
	
	//Release Lock
	if(LockHandleRelease(inode_lock) != SYNC_SUCCESS)
	{
		printf("Error: Inode Lock Unable to Release\n");
		return DFS_FAIL;
	}

	//No Inodes are available
	if(i == DFS_INODE_MAX_NUM)
	{
		printf("Error: Max Capacity of Inodes have been reached\n");
		return DFS_FAIL;
	}
	return inode_handle;
}


//-----------------------------------------------------------------
// DfsInodeDelete de-allocates any data blocks used by this inode, 
// including the indirect addressing block if necessary, then mark 
// the inode as no longer in use. Use locks when modifying the 
// "inuse" flag in an inode.Return DFS_FAIL on failure, and 
// DFS_SUCCESS on success.
//-----------------------------------------------------------------

int DfsInodeDelete(uint32 handle) {
	return DFS_FAIL;
}


//-----------------------------------------------------------------
// DfsInodeReadBytes reads num_bytes from the file represented by 
// the inode handle, starting at virtual byte start_byte, copying 
// the data to the address pointed to by mem. Return DFS_FAIL on 
// failure, and the number of bytes read on success.
//-----------------------------------------------------------------

int DfsInodeReadBytes(uint32 handle, void *mem, int start_byte, int num_bytes) {
	return DFS_FAIL;
}


//-----------------------------------------------------------------
// DfsInodeWriteBytes writes num_bytes from the memory pointed to 
// by mem to the file represented by the inode handle, starting at 
// virtual byte start_byte. Note that if you are only writing part 
// of a given file system block, you'll need to read that block 
// from the disk first. Return DFS_FAIL on failure and the number 
// of bytes written on success.
//-----------------------------------------------------------------

int DfsInodeWriteBytes(uint32 handle, void *mem, int start_byte, int num_bytes) 
{
	uint32 block;
	uint32 i;
	dfs_block d_block;
	dfs_block recieving_block;
	uint32 ret;
	
	char str1[20] = "Hello World!\n";
	
	block = DfsAllocateBlock();
	
	dstrcpy(d_block.data,str1);

	DfsWriteBlock(block, &d_block);	
	

	printf("Closing File System!\n");
	DfsCloseFileSystem();

	printf("Now we reopen file system!\n");
	DfsOpenFileSystem();

	DfsReadBlock(block, &recieving_block);
	
	printf("Recieving Block now has %s\n", recieving_block.data);

	//dstrcpy(dest, src
	return DFS_FAIL;
}


//-----------------------------------------------------------------
// DfsInodeFilesize simply returns the size of an inode's file. 
// This is defined as the maximum virtual byte number that has 
// been written to the inode thus far. Return DFS_FAIL on failure.
//-----------------------------------------------------------------

uint32 DfsInodeFilesize(uint32 handle) {
	if(DfsCheckSystem() == DFS_FAIL)
	{
		return DFS_FAIL;
	}
	return inodes[handle].size;
}


//-----------------------------------------------------------------
// DfsInodeAllocateVirtualBlock allocates a new filesystem block 
// for the given inode, storing its blocknumber at index 
// virtual_blocknumber in the translation table. If the 
// virtual_blocknumber resides in the indirect address space, and 
// there is not an allocated indirect addressing table, allocate it. 
// Return DFS_FAIL on failure, and the newly allocated file system 
// block number on success.
//-----------------------------------------------------------------

uint32 DfsInodeAllocateVirtualBlock(uint32 handle, uint32 virtual_blocknum) 
{

	DfsAllocateBlock();
	return DFS_FAIL;

}



//-----------------------------------------------------------------
// DfsInodeTranslateVirtualToFilesys translates the 
// virtual_blocknum to the corresponding file system block using 
// the inode identified by handle. Return DFS_FAIL on failure.
//-----------------------------------------------------------------

uint32 DfsInodeTranslateVirtualToFilesys(uint32 handle, uint32 virtual_blocknum) {
	return DFS_FAIL;
}

