#ifndef __FILES_SHARED__
#define __FILES_SHARED__

#define FILE_SEEK_SET 1
#define FILE_SEEK_END 2
#define FILE_SEEK_CUR 3

#define FILE_MAX_FILENAME_LENGTH 44

#define FILE_MAX_READWRITE_BYTES 4096


enum mode_status
{
	r,
	w,
	rw,
	undefined
};
typedef struct file_descriptor {
  // STUDENT: put file descriptor info here
	char  filename[FILE_MAX_FILENAME_LENGTH];

	uint32 inode_handle;
	uint32 current_position;
	bool eof;
	bool open;

	//Indicates mode we are in
	enum mode_status mode; 
	
	//8 bit file permission vector
	char file_permissions;
	int owner_id;
} file_descriptor;

#define FILE_FAIL -1
#define FILE_EOF -1
#define FILE_SUCCESS 1

#endif
