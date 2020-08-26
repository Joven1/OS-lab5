#include "ostraps.h"
#include "dlxos.h"
#include "traps.h"
#include "disk.h"
#include "dfs.h"

#define BUFFSIZE 2048

void RunOSTests() {
  // STUDENT: run any os-level tests here
  // int i = 0;
  uint32 handleMeep, handleMarp, handleLong, handleDerp;
  char writebuff[BUFFSIZE] = "Hello, World!", readbuff[BUFFSIZE];

  /* FSDRIVER BASED TESTS
  printf("RunOSTests: Allocating 16500 blocks\n");
  for (i = 0; i < 16500; i++) {
  	printf("RunOSTests: Allocating block %d\n",  i);
  	if (DfsAllocateBlock() == DFS_FAIL) {
  		printf("RunOSTests: Allocation failed\n");
  		break;
  	}
  }

  printf("RunOSTests: Attempting to free block 0 (metadata)\n");
  DfsFreeBlock(0);

  printf("RunOSTests: Attempting to free block 20 (metadata)\n");
  DfsFreeBlock(20);

  printf("RunOSTests: Attempting to free a block outside the file system\n");
  DfsFreeBlock(24000);

  printf("RunOSTests: Attempting to free a valid block\n");
  DfsFreeBlock(21);

  printf("RunOSTests: Attempting to double free a block\n");
  DfsFreeBlock(21);
  */

  /* INODE BASED TESTS */
  // Checks some existences
  DfsInodeFilenameExists("meep.txt");
  DfsInodeFilenameExists("marp.txt");
  DfsInodeFilenameExists("areallylongfilenamethisistolongtobeinaninodesothereshouldbeanerror.txt");

	printf("Made it to INODE SCENARIO 0\n\n");
  /******** INODE SCENARIO 0: Tests Opening inodes, including ones with filenames that are too long ********/
  if ((handleMeep = DfsInodeOpen("meep.txt")) == DFS_FAIL) {
    printf("ostests: Opening meep.txt failed!\n");
    GracefulExit();
  }


  if((handleMarp = DfsInodeOpen("marp.txt")) == DFS_FAIL) {
    printf("ostests: Opening marp.txt failed!\n");
    GracefulExit();
  }

  if ((handleDerp = DfsInodeOpen("herpaderp.txt")) == DFS_FAIL) {
    printf("ostests: Opening herpaderp.txt failed!\n");
    GracefulExit();
  }

  // This should fail for being too long
  if ((handleLong = DfsInodeOpen("areallylongfilenamethisistolongtobeinaninodesothereshouldbeanerror.txt")) != DFS_FAIL) {
    printf("ostests: opening areallylongfilenamethisistolongtobeinaninodesothereshouldbeanerror.txt succeeded!\n");
    GracefulExit();
  }

  printf("\n\n");

  /******** INODE SCENARIO 1: Attempts writing and reading a simple string from the first byte position of an inode, as well as including file size verification ********/
  printf("ostests: Starting INODE SCENARIO 1\n");
  if (DfsInodeFilesize(handleMeep) != 0) {
    printf("ostests: INODE SCENARIO 1 - inode %u size is %d, should be 0!\n", handleMeep, DfsInodeFilesize(handleMeep));
    GracefulExit();
  }

  // Attempts to write to a file at the start of the file
  if (DfsInodeWriteBytes(handleMeep, writebuff, 0, dstrlen(writebuff)) != dstrlen(writebuff)) {
    printf("ostests: INODE SCENARIO 1 - failed to write %s to inode %u\n", writebuff, handleMeep);
    GracefulExit();
  }

  // Attempts to read from that same file at the start of the file
  if (DfsInodeReadBytes(handleMeep, readbuff, 0, dstrlen(writebuff)) != dstrlen(writebuff)) {
    printf("ostests: INODE SCENARIO 1 - failed to read from inode %u\n", handleMeep);
    GracefulExit();
  }

  if (dstrncmp(readbuff, writebuff, dstrlen(writebuff)) != 0) {
    printf("ostests: INODE SCENARIO 1 - did not read data correctly from inode %u\n", handleMeep);
    GracefulExit();
  }

  if (DfsInodeFilesize(handleMeep) != dstrlen(readbuff)) {
    printf("ostests: INODE SCENARIO 1 - handle %u size is %d, should be %d!\n", handleMeep, DfsInodeFilesize(handleMeep), dstrlen(readbuff));
    GracefulExit();
  }

  printf("ostests: INODE SCENARIO 1 succeeds: readbuff = %s, size of meep.txt = %d\n", readbuff, DfsInodeFilesize(handleMeep));

  printf("\n\n");

  /******** INODE SCENARIO 2: Attempts writing non-contiguous parts of file and verifying maxbyte ********/
  // Test plan:
  //  1) Write into the file we wrote into in scenario 1. This write will go into a different block than the first write,
  //     but will still be aligned to byte 0 of the block
  //  2) Read this string back and verify that it is what we believe it to be
  //  3) Read back the original write from scenario 1 to make sure it is still fine
  //  4) Read the file size.
  //  4) Write to a location in between the two writes from scenario 1 and 2 to make sure that the filesize stays the same
  //  5) Read the file size. Even though we have only written a few bytes, the maxbyte should still be the end of the write in step 1

  bzero(readbuff, BUFFSIZE);
  printf("ostests: Starting INODE SCENARIO 2\n");
  // Attempts to write to file at the start of a different block
  // This scenario assumes a DFS_BLOCKSIZE blocksize
  if (DfsInodeWriteBytes(handleMeep, writebuff, DFS_BLOCKSIZE * 2, dstrlen(writebuff)) != dstrlen(writebuff)) {
    printf("ostests: INODE SCENARIO 2 - failed to write %s to inode %u at position %d\n", writebuff, handleMeep, DFS_BLOCKSIZE * 2);
    GracefulExit();
  }

  if (DfsInodeReadBytes(handleMeep, readbuff, DFS_BLOCKSIZE * 2, dstrlen(writebuff)) != dstrlen(writebuff)) {
    printf("ostests: INODE SCENARIO 2 - failed to read from inode %u at position %d\n", handleMeep, DFS_BLOCKSIZE * 2);
    GracefulExit();
  }

  if (dstrncmp(readbuff, writebuff, dstrlen(writebuff)) != 0) {
    printf("ostests: INODE SCENARIO 2 - did not read data correctly from inode %u\n, actual = %s\n", handleMeep, readbuff);
    GracefulExit();
  }

  bzero(readbuff, BUFFSIZE);

  if (DfsInodeReadBytes(handleMeep, readbuff, 0, dstrlen(writebuff)) != dstrlen(writebuff)) {
    printf("ostests: INODE SCENARIO 2 - failed to read from inode %u at position %d\n", handleMeep, 0);
    GracefulExit();
  }

  if (dstrncmp(readbuff, writebuff, dstrlen(writebuff)) != 0) {
    printf("ostests: INODE SCENARIO 2 - did not read data correctly from inode %u\n, actual = %s\n", handleMeep, readbuff);
    GracefulExit();
  }


  // This scenario assumes that this is writing into the last position in the file, and therefore the filesize of the inode
  // should be the base + num_bytes of this write
  if (DfsInodeFilesize(handleMeep) != ((DFS_BLOCKSIZE * 2) + dstrlen(writebuff))) {
    printf("ostests: INODE SCENARIO 2 - handle %u size is %d, should be %d!\n", handleMeep, DfsInodeFilesize(handleMeep), ((DFS_BLOCKSIZE * 2) + dstrlen(writebuff)));
    GracefulExit();
  }

  if (DfsInodeWriteBytes(handleMeep, writebuff, DFS_BLOCKSIZE * 1, dstrlen(writebuff)) != dstrlen(writebuff)) {
    printf("ostests: INODE SCENARIO 2 - failed to write %s to inode %u at position %d\n", writebuff, handleMeep, DFS_BLOCKSIZE * 1);
    GracefulExit();
  }

  if (DfsInodeFilesize(handleMeep) != ((DFS_BLOCKSIZE * 2) + dstrlen(writebuff))) {
    printf("ostests: INODE SCENARIO 2 - handle %u size is %d, should be %d!\n", handleMeep, DfsInodeFilesize(handleMeep), ((DFS_BLOCKSIZE * 2) + dstrlen(writebuff)));
    GracefulExit();
  }

  printf("ostests: INODE SCENARIO 2 succeeds: readbuff = %s, size of meep.txt = %d\n", readbuff, DfsInodeFilesize(handleMeep));

  printf("\n\n");

  /******** INODE SCENARIO 3: Attempts writing non block-aligned parts of a file ********/
  // Test plan:
  //  1) Write to a block that has already been allocated not at the beginning of the block
  //  2) Read from the block to make sure that the data is still the same
  //  3) Write to a block that has not already been allocated on a non-aligned space
  //  4) Repeat step 2 for this block

  bzero(readbuff, BUFFSIZE);
  printf("ostests: Starting INODE SCENARIO 3\n");
  // Attempts to write to file at the start of a different block
  // This scenario assumes a DFS_BLOCKSIZE blocksize
  if (DfsInodeWriteBytes(handleMeep, writebuff, 357, dstrlen(writebuff)) != dstrlen(writebuff)) {
    printf("ostests: INODE SCENARIO 3 - failed to write %s to inode %u at position %d\n", writebuff, handleMeep, 357);
    GracefulExit();
  }

  if (DfsInodeReadBytes(handleMeep, readbuff, 357, dstrlen(writebuff)) != dstrlen(writebuff)) {
    printf("ostests: INODE SCENARIO 3 - failed to read from inode %u at position %d\n", handleMeep, 357);
    GracefulExit();
  }

  if (dstrncmp(readbuff, writebuff, dstrlen(writebuff)) != 0) {
    printf("ostests: INODE SCENARIO 3 - did not read data correctly from inode %u\n, actual = %s\n", handleMeep, readbuff);
    GracefulExit();
  }

  bzero(readbuff, BUFFSIZE);

  if (DfsInodeWriteBytes(handleMeep, writebuff, (DFS_BLOCKSIZE * 3) + 357, dstrlen(writebuff)) != dstrlen(writebuff)) {
    printf("ostests: INODE SCENARIO 3 - failed to write %s to inode %u at position %d\n", writebuff, handleMeep, (DFS_BLOCKSIZE * 3) + 357);
    GracefulExit();
  }

  if (DfsInodeReadBytes(handleMeep, readbuff, 357, dstrlen(writebuff)) != dstrlen(writebuff)) {
    printf("ostests: INODE SCENARIO 3 - failed to read from inode %u at position %d\n", handleMeep, (DFS_BLOCKSIZE * 3) + 357);
    GracefulExit();
  }

  if (dstrncmp(readbuff, writebuff, dstrlen(writebuff)) != 0) {
    printf("ostests: INODE SCENARIO 3 - did not read data correctly from inode %u\n, actual = %s\n", handleMeep, readbuff);
    GracefulExit();
  }

  printf("ostests: INODE SCENARIO 3 succeeds: readbuff = %s, size of meep.txt = %d\n", readbuff, DfsInodeFilesize(handleMeep));

  printf("\n\n");

  /******** INODE SCENARIO 4: Attempts writing across block boundaries ********/
  // Test plan:
  //  1) Write to a block such that the contents of the write will cross over
  //     into the next block
  //  2) Read the data and verify that the data has been read correctly

  bzero(readbuff, BUFFSIZE);
  printf("ostests: Starting INODE SCENARIO 4\n");
  // Attempts to write to file at the end of a block
  // This scenario assumes a DFS_BLOCKSIZE blocksize
  if (DfsInodeWriteBytes(handleMeep, writebuff, 1020, dstrlen(writebuff)) != dstrlen(writebuff)) {
    printf("ostests: INODE SCENARIO 4 - failed to write %s to inode %u at position %d\n", writebuff, handleMeep, 1020);
    GracefulExit();
  }

  if (DfsInodeReadBytes(handleMeep, readbuff, 1020, dstrlen(writebuff)) != dstrlen(writebuff)) {
    printf("ostests: INODE SCENARIO 4 - failed to read from inode %u at position %d\n", handleMeep, 1020);
    GracefulExit();
  }

  if (dstrncmp(readbuff, writebuff, dstrlen(writebuff)) != 0) {
    printf("ostests: INODE SCENARIO 4 - did not read data correctly from inode %u\n, actual = %s\n", handleMeep, readbuff);
    GracefulExit();
  }

  printf("ostests: INODE SCENARIO 4 succeeds - readbuff = %s, size of meep.txt = %d\n", readbuff, DfsInodeFilesize(handleMeep));

  printf("\n\n");

  /******** INODE SCENARIO 5: Attempts writing into the indirect addressing space ********/
  // Test plan:
  //  1) Write to a block such that the contents of the write will be indirectly addressed
  //     by the inode
  //  2) Read the data and verify that it has been written correctly

  bzero(readbuff, BUFFSIZE);
  printf("ostests: Starting INODE SCENARIO 5\n");
  // This scenario assumes a DFS_BLOCKSIZE blocksize
  if (DfsInodeWriteBytes(handleMeep, writebuff, (DFS_BLOCKSIZE * 11), dstrlen(writebuff)) != dstrlen(writebuff)) {
    printf("ostests: INODE SCENARIO 5 - failed to write %s to inode %u at position %d\n", writebuff, handleMeep, (DFS_BLOCKSIZE * 11));
    GracefulExit();
  }

  if (DfsInodeReadBytes(handleMeep, readbuff, (DFS_BLOCKSIZE * 11), dstrlen(writebuff)) != dstrlen(writebuff)) {
    printf("ostests: INODE SCENARIO 5 - failed to read from inode %u at position %d\n", handleMeep, (DFS_BLOCKSIZE * 11));
    GracefulExit();
  }

  if (dstrncmp(readbuff, writebuff, dstrlen(writebuff)) != 0) {
    printf("ostests: INODE SCENARIO 5 - did not read data correctly from inode %u\n, actual = %s\n", handleMeep, readbuff);
    GracefulExit();
  }

  printf("ostests: INODE SCENARIO 5 succeeds - readbuff = %s, size of meep.txt = %d\n", readbuff, DfsInodeFilesize(handleMeep));

  printf("\n\n");

  /******** INODE SCENARIO 6: Attempts writing into the end of the last direct block, into the indirectly addressed space ********/
  // Test plan: Repeat scenario 4, except start in the last direct block and write into the first indirect block

  bzero(readbuff, BUFFSIZE);
  printf("ostests: Starting INODE SCENARIO 6\n");
  // Attempts to write to file at the end of a block
  // This scenario assumes a DFS_BLOCKSIZE blocksize
  if (DfsInodeWriteBytes(handleMeep, writebuff, (DFS_BLOCKSIZE * 9) + 1020, dstrlen(writebuff)) != dstrlen(writebuff)) {
    printf("ostests: INODE SCENARIO 6 - failed to write %s to inode %u at position %d\n", writebuff, handleMeep, (DFS_BLOCKSIZE * 9) + 1020);
    GracefulExit();
  }

  if (DfsInodeReadBytes(handleMeep, readbuff, (DFS_BLOCKSIZE * 9) + 1020, dstrlen(writebuff)) != dstrlen(writebuff)) {
    printf("ostests: INODE SCENARIO 6 - failed to read from inode %u at position %d\n", handleMeep, (DFS_BLOCKSIZE * 9) + 1020);
    GracefulExit();
  }

  if (dstrncmp(readbuff, writebuff, dstrlen(writebuff)) != 0) {
    printf("ostests: INODE SCENARIO 6 - did not read data correctly from inode %u\n, actual = %s\n", handleMeep, readbuff);
    GracefulExit();
  }

  printf("ostests: INODE SCENARIO 6 succeeds - readbuff = %s, size of meep.txt = %d\n", readbuff, DfsInodeFilesize(handleMeep));

  printf("\n\n");

  /******** INODE SCENARIO 7: Write more than a block of data ********/
  // Test plan:
  //  1) Write content that is larger than the block size
  //  2) Try to read it back

  bzero(readbuff, BUFFSIZE);
  printf("ostests: Starting INODE SCENARIO 7\n");
  // Creates the string

  dstrncpy(writebuff, "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Morbi vestibulum, massa nec porta blandit, nibh turpis faucibus sapien, nec finibus eros nibh a massa. Aliquam sagittis ligula eu ipsum dictum volutpat. Curabitur quis est nec purus sollicitudin sollicitudin consequat ac ex. Suspendisse urna urna, viverra et magna in, pharetra venenatis quam. Phasellus purus est, pharetra quis lacinia at, semper ac ipsum. Aliquam pharetra et tellus et lacinia. Phasellus elit ante, egestas et elementum ac, malesuada eu magna. In viverra libero sed ipsum interdum, vel dictum nibh condimentum. Duis sit amet ligula non leo pharetra scelerisque. Etiam nisl magna, ultricies vitae pulvinar ac, interdum mollis lectus. Curabitur porta sem aliquet sagittis dignissim. Fusce sapien purus, convallis dignissim placerat nec, hendrerit ac nibh. Nunc eleifend euismod convallis. Aliquam sed nisi suscipit, vulputate nunc eget, mollis turpis."
                      "Ut ornare ligula ac quam convallis pharetra. Praesent fermentum, dui ut posuere fringilla, augue eros euismod arcu, non sollicitudin eros magna at ante. Integer id sapien tempus, dictum nibh vitae, finibus sem. In non venenatis nisi. Donec eu nisl dictum, vehicula nunc nec, porttitor ante. Integer consectetur ut neque vitae aliquam. Duis at convallis dui."
                      "Mauris viverra sed mi quis vehicula. Nullam augue augue, mollis quis tempus sodales, molestie quis mi. Nam ac risus sed nibh faucibus commodo. Sed sollicitudin augue at venenatis tristique. Integer volutpat eros vitae velit lobortis rutrum. Aenean metus.",
                      1536);


  // This scenario assumes a DFS_BLOCKSIZE blocksize
  if (DfsInodeWriteBytes(handleMeep, writebuff, 0, dstrlen(writebuff)) != dstrlen(writebuff)) {
    printf("ostests: INODE SCENARIO 7 - failed to write %s to inode %u at position %d\n", writebuff, handleMeep, 0);
    GracefulExit();
  }

  if (DfsInodeReadBytes(handleMeep, readbuff, 0, dstrlen(writebuff)) != dstrlen(writebuff)) {
    printf("ostests: INODE SCENARIO 7 - failed to read from inode %u at position %d\n", handleMeep, 0);
    GracefulExit();
  }

  if (dstrncmp(readbuff, writebuff, dstrlen(writebuff)) != 0) {
    printf("ostests: INODE SCENARIO 7 - did not read data correctly from inode %u\n, actual = %s\n", handleMeep, readbuff);
    GracefulExit();
  }

  printf("ostests: INODE SCENARIO 7 succeeds - readbuff = %s, size of meep.txt = %d\n", readbuff, DfsInodeFilesize(handleMeep));

  printf("\n\n");
  
  // Attempts to delete a couple of the files and then open one again
  if (DfsInodeDelete(handleMeep) != DFS_SUCCESS) {
    printf("ostests: deleting inode %u (meep.txt) failed\n", handleMeep);
    GracefulExit();
  }

  if (DfsInodeDelete(handleMarp) != DFS_SUCCESS) {
    printf("ostests: deleting inode %u (marp.txt) failed\n", handleMarp);
    GracefulExit();
  }

  // This should fail because the inode handle is DFS_FAIL
  if (DfsInodeDelete(handleLong) != DFS_FAIL) {
    printf("ostests: deleting inode %u succeeded\n", handleLong);
    GracefulExit();
  }

  // This should also fail because the inode has already been freed
  if (DfsInodeDelete(handleMeep) != DFS_FAIL) {
    printf("ostests: deleting inode %u (meep.txt - ALREADY DELETED) succeeded\n", handleMeep);
    GracefulExit();
  }
  
  return;
}

