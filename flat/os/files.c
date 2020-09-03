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


void FileModuleInit()
{
	int i;

	//Set the starting conditions for files
	for(i = 0; i < FILE_MAX_OPEN_FILES; i++)
	{
		//Empty out filename
		bzero(files[i].filename, FILE_MAX_FILENAME_LENGTH);	
		files[i].inode_handle = 0;
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
	
	if(inode_handle != DFS_FAIL)
	{
		//Loop through all files to see if inode handle is taken
		for(i = 0; i < FILE_MAX_OPEN_FILES; i++)
		{
			if(files[i].inode_handle == inode_handle)
			{
				file_handle = i;
				break;
			}	
		}
	
		//Check if the current process has ownership of file
		if(file_handle != FILE_FAIL && (GetCurrentPid() != files[file_handle].owner_id))
		{
			printf("I AM THE RUNNER\n\n\n");
			//Release File Lock
			if(LockHandleRelease(file_lock) != SYNC_SUCCESS)
			{
				printf("Error: File Lock Release Failed!\n");	
				return FILE_FAIL;
			}		

			return FILE_FAIL;
		}
	}	

	//If we are, not in writing mode
	if(file_mode != r)
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

		//Allocate an inode
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
			if(files[i].inode_handle == 0)
			{
				file_handle = i;
			}
		}	

	}
	
	//Set the rest of the parameters
	dstrncpy(files[file_handle].filename, filename, dstrlen(filename));
	files[file_handle].inode_handle = inode_handle;
	files[file_handle].owner_id = GetCurrentPid();
	printf("File handle is %d\n", file_handle);
	printf("File Ownder is %d\n", files[file_handle].owner_id);	
	files[file_handle].mode = file_mode;

	//Release File Lock
	if(LockHandleRelease(file_lock) != SYNC_SUCCESS)
	{
		printf("Error: File Lock Release Failed!\n");	
		return FILE_FAIL;
	}	

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
	bzero(files[handle].filename, FILE_MAX_FILENAME_LENGTH);	
	files[handle].inode_handle = 0;
	files[handle].current_position = 0;
	files[handle].eof = 0;
	files[handle].mode = undefined;
	files[handle].file_permissions = 0;
	files[handle].owner_id = -1;
	files[handle].open = false;


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
	
	//Verify the process has authority over file
	if(GetCurrentPid() != files[handle].owner_id)
	{
		printf("Error: Process (%d) has no ownership!\n", GetCurrentPid());
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
	return FILE_FAIL;
}

int FileDelete(char * filename)
{
	return FILE_FAIL;
}

