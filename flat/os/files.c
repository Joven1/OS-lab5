#include "ostraps.h"
#include "dlxos.h"
#include "process.h"
#include "dfs.h"
#include "files.h"
#include "synch.h"

// You have already been told about the most likely places where you should use locks. You may use 
// additional locks if it is really necessary.

// STUDENT: put your file-level functions here
uint32 FileOpen(char * filename, char * mode)
{
	return FILE_FAIL;
}

int FileClose(int handle)
{
	return FILE_FAIL;
}

int FileRead(int handle, void * mem, int num_bytes)
{
	return FILE_FAIL;
}

int FileWrite(int handle, void * mem, int num_bytes)
{
	return FILE_FAIL;
}

int FileSeek(int handle, int num_bytes, int from_where)
{
	return FILE_FAIL;
}

int FileDelete(char * filename)
{
	return FILE_FAIL;
}

