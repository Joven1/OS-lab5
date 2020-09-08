This is my implementation of lab 5 for ECE 469. 

Note: The file which represents the "hard disk" is under "/tmp/ee469g00.img"

### Building and Running:
In order to run this lab, simply cd into flat and execute the ./run.sh script. This will first compile the os code. Afterwards it will format the .img file hosting all the memory blocks. Then it will run the test code for the os and then the test code for the file api. 

### Part 1: New File System
For this lab, we use 192 inodes each the size of 96 bytes. This will require 18432 bytes of memory or 18 blocks on disk rather than 16 as seen in the example for the handout. In this program, it simply initializes the original super block via its constants, inodes, and free block vector. The convention I used in this lab for the free block vector is '1' as inuse and '0' as not inuse to ease any confusion.

### Part 2: DFS File System Driver
The file system driver is located inside of dfs.c. Above, you'll see helper functions to print out the super block, inode, and data blocks. For this lab, I had the program use a lock during both reading and writing operations. My reasoning behind this is that even if a process is just reading a block and not making any adjustments to the shared memory, another process may be writing to that same block causing an inaccurate read. Before each function executes, DfsCheckSystem() runs and will return DFS_FAIL if the file system is invalid or not open. Each function in the file system driver automatically assumes that the super block was initialized properly and will not run otherwise. Additional test cases are introduced under OS_TESTS(), not to be confused with os_tests inside of ostests.c.

### Part 3: File-Based API
Lastly, the file api we are familiar with in most C code is implemented under files.c. 

Assumption: In this case, there are only three file modes, "r", "w", "rw". In the case of mode "r", file bytes are read into a memory pointer. In the case of more "w", if the file doesn't exist, open a new one, and if it does exist, delete it and start over. Lastly, for "rw", it has the same functionality as both modes, except this time, it does not delete the file if it exists. 

Same logic from the DFS File System Driver applies. During any read and write operation, a lock is required to prevent any faulty reads. Another assumption that was made, is that underneath include/os/files.h, the FILE_MAX_OPEN_FILES was defined as 15. I believe this just simply means that not more than 15 files could be open at a time, even though the max number of inodes to represent files is 192. Test code for the functionality of the file api is under apps/file_test/file_test/file_test.c. 

### External Resources:
I used the blockprint and display_disk scripts to do most of the debugging on the disk for this lab. Display disk simply calls blockprint when printing out the superblock, inodes, and free block vector.
