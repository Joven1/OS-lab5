#include "ostraps.h"
#include "dlxos.h"
#include "process.h"
#include "dfs.h"
#include "files.h"
#include "synch.h"

// You have already been told about the most likely places where you should use locks. You may use 
// additional locks if it is really necessary.
static file_descriptor files[FILE_MAX_OPEN_FILES];
static lock_t file_lock;

void PrintFileStatus(int handle)
{
	printf("File Handle: %d\n", handle);
	printf("Filename: %s\n", files[handle].filename);
	printf("Inode Handle: %d\n", files[handle].inode_handle);
	printf("Owner ID: %d\n", files[handle].owner_id);
	printf("Current Position: %d\n", files[handle].current_position);
	printf("\n\n\n");
}
void FileModuleInit()
{
	int i;

	//Set the starting conditions for files
	for(i = 0; i < FILE_MAX_OPEN_FILES; i++)
	{
		//Empty out filename
		bzero(files[i].filename, FILE_MAX_FILENAME_LENGTH);	
		files[i].inode_handle = -1;
		files[i].current_position = 0;
		files[i].eof = 0;
		files[i].mode = undefined;
		files[i].file_permissions = 0;
		files[i].owner_id = -1;
	}
	
	//Initialize file lock
	file_lock = LockCreate();
}

// STUDENT: put your file-level functions here
bool isOpen(char * filename)
{
	uint32 i;
	for(i = 0; i < DFS_INODE_MAX_NUM; i++)
	{
		printf("%d\n", i);
	}
	return true;
}

int getFileMode(char * mode)
{
	if(dstrlen(mode) > 2)
	{
		return undefined;
	}
	
	if(dstrlen(mode) == 1)
	{
		if(mode[0] == 'r')
		{
			return r;
		}
		else if(mode[0] == 'w')
		{
			return w;
		}
	}

	if(dstrlen(mode) == 2)
	{
		if(mode[0] == 'r' && mode[1] == 'w')
		{
			return rw;
		}
	}
	return undefined;
}

void NullifyFileEntry(uint32 handle)
{
	//Nullify the file entry (note: this does not delete the data)
	bzero(files[handle].filename, FILE_MAX_FILENAME_LENGTH);	
	files[handle].inode_handle = -1; //Note: if we set this to 0, what if the inode array file exists returns 0? Don't :)
	files[handle].current_position = 0;
	files[handle].eof = false;
	files[handle].mode = undefined;
	files[handle].file_permissions = 0;
	files[handle].owner_id = -1;
}

uint32 FileOpen(char * filename, char * mode)
{
	int inode_handle;
	int file_mode = getFileMode(mode);
	uint32 file_handle = FILE_FAIL;
	uint32 i;

	//Acquire File Lock
	if(LockHandleAcquire(file_lock) != SYNC_SUCCESS)
	{
		printf("Error: File Lock Acquisition failed!\n");
		return FILE_FAIL;
	}

	if(file_mode == undefined)
	{
		printf("Error: Unknown file mode %s\n", mode);

		//Release File Lock
		if(LockHandleRelease(file_lock) != SYNC_SUCCESS)
		{
			printf("Error: File Lock Release Failed!\n");	
			return FILE_FAIL;
		}	

		return FILE_FAIL;
	}
	//Check if file is already open
	inode_handle = DfsInodeFilenameExists(filename);
	
	//If the file does exist, check if it's currently in one of our opened files
	if(inode_handle != DFS_FAIL)
	{
		for(i = 0; i < FILE_MAX_OPEN_FILES; i++)
		{
			//Check each corresponding handle	
			if(files[i].inode_handle == inode_handle)
			{
				//Check if proper owner is opening file
				if(files[i].owner_id != GetCurrentPid())
				{		

					printf("Error: Permission Denied, wrong owner %d\n", GetCurrentPid());
					
					//Release File Lock
					if(LockHandleRelease(file_lock) != SYNC_SUCCESS)
					{
						printf("Error: File Lock Release Failed!\n");	
						return FILE_FAIL;
					}			
				}
				//Set our file handle equal to our entry in the file table
				file_handle = i;
				break;	
			}
		}
	}
	
	//Overwrite the file, if we are in read mode
	//ASSUMPTION: rw doesn't overwrite the file
	if(file_mode == w)
	{
		//If the file already exists, delete the previous one
		if(inode_handle != DFS_FAIL)
		{
			if(DfsInodeDelete(inode_handle) != DFS_SUCCESS)
			{
				//Release File Lock	
				if(LockHandleRelease(file_lock) != SYNC_SUCCESS)
				{			
					printf("Error: File Lock Release Failed!\n");	
					return FILE_FAIL;
				}
				return FILE_FAIL;
			}
		}

		//Allocate a new inode
		inode_handle = DfsInodeOpen(filename);
		if(inode_handle == DFS_FAIL)
		{
			//Release File Lock
			if(LockHandleRelease(file_lock) != SYNC_SUCCESS)
			{
				printf("Error: File Lock Release Failed!\n");	
				return FILE_FAIL;
			}	
			return FILE_FAIL;
		}
	}

	//If file has not been allocated, search for a free slot
	if(file_handle == FILE_FAIL)
	{
		for(i = 0; i < FILE_MAX_OPEN_FILES; i++)
		{
			if(files[i].inode_handle == -1)
			{
				file_handle = i;
				break;
			}
		}	

	}

	//If we haven't found it, max opened files has been reached
	if(file_handle == FILE_FAIL)
	{
		printf("Error Maximum Opened Files Reached!\n");
		//Release File Lock
		if(LockHandleRelease(file_lock) != SYNC_SUCCESS)
		{
			printf("Error: File Lock Release Failed!\n");	
			return FILE_FAIL;
		}	
	}


	//Set the rest of the parameters
	dstrncpy(files[file_handle].filename, filename, dstrlen(filename));
	files[file_handle].inode_handle = inode_handle;
	files[file_handle].owner_id = GetCurrentPid();
	files[file_handle].mode = file_mode;
	files[file_handle].current_position = 0;
	files[file_handle].eof = false;

	return file_handle;
}

int FileClose(int handle)
{
	//Verify the process has authority over file
	if(GetCurrentPid() != files[handle].owner_id)
	{
		printf("Error: Process (%d) has no ownership!\n", GetCurrentPid());
		return FILE_FAIL;
	}

	//Acquire File Lock
	if(LockHandleAcquire(file_lock) != SYNC_SUCCESS)
	{
		printf("Error: File Lock Acquisition failed!\n");
		return FILE_FAIL;
	}


	//Nullify the file entry (note: this does not delete the data)
	NullifyFileEntry(handle);

	//Release File Lock
	if(LockHandleRelease(file_lock) != SYNC_SUCCESS)
	{
		printf("Error: File Lock Release Failed!\n");	
		return FILE_FAIL;
	}	


	return FILE_SUCCESS;
}

int FileRead(int handle, void * mem, int num_bytes)
{
	int bytes_read;
	//Verify the process has authority over file
	if(GetCurrentPid() != files[handle].owner_id)
	{
		printf("Error: Process (%d) has no ownership!\n", GetCurrentPid());
		return FILE_FAIL;
	}

	//Verify we are in the right mode
	if((files[handle].mode != r) && (files[handle].mode != rw))
	{
		printf("Error: Can't Read, Wrong Mode %d\n", files[handle].mode);
		return FILE_FAIL;
	}

	//Verify the input bytes
	if(num_bytes > FILE_MAX_READWRITE_BYTES)
	{
		printf("Error: Too Many Bytes (%d), max is %d\n", num_bytes, FILE_MAX_READWRITE_BYTES);
		return FILE_FAIL;
	}

	//Acquire File Lock
	if(LockHandleAcquire(file_lock) != SYNC_SUCCESS)
	{
		printf("Error: File Lock Acquisition failed!\n");
		return FILE_FAIL;
	}

	//Check if the eof flag will be set
	if((files[handle].current_position + num_bytes) > DfsInodeFilesize(files[handle].inode_handle))
	{
		files[handle].eof = true;
		//Update num bytes so we get no errors over reading
		num_bytes = DfsInodeFilesize(files[handle].inode_handle) - files[handle].current_position;
	}

	//Read in bytes to mem
	bytes_read = DfsInodeReadBytes(files[handle].inode_handle, mem, files[handle].current_position, num_bytes);
	
	//Check if operation failed or not
	if(bytes_read == DFS_FAIL)
	{
		printf("Error: Read Operation Failed!\n");	
		//Release File Lock
		if(LockHandleRelease(file_lock) != SYNC_SUCCESS)
		{
			printf("Error: File Lock Release Failed!\n");	
			return FILE_FAIL;
		}	

		return FILE_FAIL;
	}	


	files[handle].current_position = files[handle].current_position + bytes_read;

	//Release File Lock
	if(LockHandleRelease(file_lock) != SYNC_SUCCESS)
	{
		printf("Error: File Lock Release Failed!\n");	
		return FILE_FAIL;
	}	

	//Return end of file flag if reached
	if(files[handle].eof)
	{
		return FILE_EOF;
	}

	return bytes_read;
}

int FileWrite(int handle, void * mem, int num_bytes)
{
	int bytes_written;
	char buffer[500];
	uint32 i;
	uint32 bytes_read;
	
	//Verify the process has authority over file
	if(GetCurrentPid() != files[handle].owner_id)
	{
		printf("Error: Process (%d) has no ownership!\n", GetCurrentPid());
		return FILE_FAIL;
	}

	//Verify we are in the right mode
	if((files[handle].mode != w) && (files[handle].mode != rw))
	{
		printf("Error: Can't Write, Wrong Mode %d\n", files[handle].mode);
		return FILE_FAIL;
	}


	//Verify the input bytes
	if(num_bytes > FILE_MAX_READWRITE_BYTES)
	{
		printf("Error: Too Many Bytes (%d), max is %d\n", num_bytes, FILE_MAX_READWRITE_BYTES);
		return FILE_FAIL;
	}

	//Acquire File Lock
	if(LockHandleAcquire(file_lock) != SYNC_SUCCESS)
	{
		printf("Error: File Lock Acquisition failed!\n");
		return FILE_FAIL;
	}


	//Set EOF if we reach the end of file
	if((files[handle].current_position + num_bytes) >= DfsInodeFilesize(files[handle].inode_handle));
	{
		files[handle].eof = true;
	} 

	//Read in bytes to mem
	bytes_written = DfsInodeWriteBytes(files[handle].inode_handle, mem, files[handle].current_position, num_bytes);
	
	//Check if operation failed or not
	if(bytes_written == DFS_FAIL)
	{
		printf("Error: Write Operation Failed!\n");	
		//Release File Lock
		if(LockHandleRelease(file_lock) != SYNC_SUCCESS)
		{
			printf("Error: File Lock Release Failed!\n");	
			return FILE_FAIL;
		}	

		return FILE_FAIL;
	}	

	files[handle].current_position = files[handle].current_position + bytes_written;
	
	//Release File Lock
	if(LockHandleRelease(file_lock) != SYNC_SUCCESS)
	{
		printf("Error: File Lock Release Failed!\n");	
		return FILE_FAIL;
	}	

	
	return bytes_written;
}

int FileSeek(int handle, int num_bytes, int from_where)
{
	//Verify the process has authority over file
	if(GetCurrentPid() != files[handle].owner_id)
	{
		printf("Error: Process (%d) has no ownership!\n", GetCurrentPid());
		return FILE_FAIL;
	}

	//Acquire File Lock
	if(LockHandleAcquire(file_lock) != SYNC_SUCCESS)
	{
		printf("Error: File Lock Acquisition failed!\n");
		return FILE_FAIL;
	}


	switch(from_where)
	{
		//Increase Position relative to current position
		case FILE_SEEK_CUR:
			files[handle].current_position = files[handle].current_position + num_bytes;
			break;
		//Set Position relative to the beginning
		case FILE_SEEK_SET:
			files[handle].current_position = num_bytes;
			break;
		//Set Position relative to end of file
		case FILE_SEEK_END:
			files[handle].current_position = DfsInodeFilesize(files[handle].inode_handle) + num_bytes;
			break;

		default:
			//Acquire File Lock
			if(LockHandleRelease(file_lock) != SYNC_SUCCESS)
			{
				printf("Error: File Lock Acquisition failed!\n");
				return FILE_FAIL;
			}
			return FILE_FAIL;


	}

	//Validate Cursor Results
	if(files[handle].current_position < 0)
	{
		return FILE_FAIL;
	}

	if(files[handle].current_position > DfsInodeFilesize(files[handle].inode_handle))
	{
		return FILE_FAIL;
	}

	//Set EOF flag if cursor hits it
	if(files[handle].current_position == DfsInodeFilesize(files[handle].inode_handle))
	{
		files[handle].eof = true;
	}
	else
	{
		files[handle].eof = false;
	}

	//Release File Lock
	if(LockHandleRelease(file_lock) != SYNC_SUCCESS)
	{
		printf("Error: File Lock Release Failed!\n");	
		return FILE_FAIL;
	}	


	return FILE_SUCCESS;
}

int FileDelete(char * filename)
{
	uint32 inode_handle;
	uint32 file_handle = DFS_FAIL;
	uint32 i;

	//Acquire File Lock
	if(LockHandleAcquire(file_lock) != SYNC_SUCCESS)
	{
		printf("Error: File Lock Acquisition failed!\n");
		return FILE_FAIL;
	}



	//Check if file has already been opened 
	
	inode_handle = DfsInodeFilenameExists(filename);
	if(inode_handle != DFS_FAIL)
	{
		for(i = 0; i < FILE_MAX_OPEN_FILES; i++)
		{
			//Check each corresponding handle	
			if(files[i].inode_handle == inode_handle)
			{
				//Check if proper owner is opening file
				if(files[i].owner_id != GetCurrentPid())
				{		

					printf("Error: Permission Denied, wrong owner %d\n", GetCurrentPid());
					
					//Release File Lock
					if(LockHandleRelease(file_lock) != SYNC_SUCCESS)
					{
						printf("Error: File Lock Release Failed!\n");	
						return FILE_FAIL;
					}			
				}
				//Set our file handle equal to our entry in the file table
				file_handle = i;	
				break;
			}
		}
	}
	else
	{
		return FILE_FAIL;
	}

	//Check if file handle was found
	if(file_handle != DFS_FAIL)
	{
		//Delete inode containing file
		if(DfsInodeDelete(inode_handle) != DFS_SUCCESS)
		{
			return FILE_FAIL;
		}
		
		//Delete the File Entry on Open File Table
		NullifyFileEntry(file_handle);		
		
	}

	//Release File Lock
	if(LockHandleRelease(file_lock) != SYNC_SUCCESS)
	{
		printf("Error: File Lock Release Failed!\n");	
		return FILE_FAIL;
	}	



	return FILE_SUCCESS;
}


































