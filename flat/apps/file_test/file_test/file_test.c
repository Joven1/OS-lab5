#include "usertraps.h"

void main (int argc, char *argv[])
{
	int retval;
	int handle;
	char *file1 = "test_file";
	char *file2 = "test_file_1";
	char data[4096];
	char read_data[4096];
	int i = 0;

	/////////////test 1
	Printf("\n\ntest1\n");
	retval = file_open(file1, "rw");
	Printf("called file_open, return %d\n", retval);

	/////////////test 2
	Printf("\n\ntest2\n");
	handle = retval;
	retval = file_close(handle);
	Printf("called file_close, return %d\n", retval);

	/////////////test 3
	Printf("\n\ntest3\n");
	retval = file_open(file1, "rw");
	Printf("called file_open, return %d\n", retval);
	handle = retval;
	for (i=0; i<2048; i++) {
		data[i] = 'A';
	}
	retval = file_write(handle, (void *)data, 512);
	Printf("called file_write, return %d\n", retval);
	retval = file_close(handle);
	Printf("called file_close, return %d\n", retval);

	/////////////test 3
	Printf("\n\ntest4\n");
	retval = file_open(file1, "rw");
	Printf("called file_open, return %d\n", retval);
	handle = retval;
	for (i=0; i<2048; i++) {
		read_data[i] = '\0';
	}
	retval = file_read(handle, (void *)read_data, 512);
	Printf("called file_read, return %d\n", retval);
	for (i=0; i<1024; i++) {
		Printf("%c", read_data[i]);
	}
	Printf("\n");
	retval = file_close(handle);
	Printf("called file_close, return %d\n", retval);

	/////////////test 5
	Printf("\n\ntest5 integreated test\n");
	retval = file_open(file2, "w");
	Printf("FILE OPEN START\n\n");	
Printf("open file - handle %d\n", retval);
	handle = retval;
	for (i=0; i<3333; i++) {
		data[i] = 'z';
	}
	retval = file_write(handle, (void *)data, 3333);
	Printf("write to file, return %d\n", retval);
	retval = file_close(handle);
	Printf("close file, return %d\n", retval);
	retval = file_open(file2, "rw");
	Printf("open same file again, return %d\n", retval);
	handle = retval;
	//empty the read buffer
	for (i=0; i<4096; i++) {
		read_data[i] = '\0';
	}
	retval = file_read(handle, (void *)read_data, 3333);
	Printf("read file into read_data, return %d\n", retval);
	Printf("print read_data content: \n");
	for (i=0; i<3333; i++) {
		Printf("%c", read_data[i]);
	}
	Printf("\n");

	/////////////test 6
	Printf("\n\ntest6 integreated test with seek\n");
	for (i=0; i<4000; i++) {
		data[i] = 'h';
	}
	retval = file_write(handle, (void *)data, 2000);
	Printf("write to file 2000, return %d\n", retval);
	retval = file_seek(handle, -2010, 3);
	Printf("file seek -2010, return %d\n", retval);
	//empty the read buffer
	for (i=0; i<4096; i++) {
		read_data[i] = '\0';
	}
	retval = file_read(handle, (void *)read_data, 2000);
	Printf("print read_data content (should only see 10 z characters): \n");
	for (i=0; i<3000; i++) {
		Printf("%c", read_data[i]);
	}
	Printf("\n");
	retval = file_close(handle);
	Printf("close file, return %d\n", retval);

	/////////////test 7
	Printf("\n\ntest7\n");
	retval = file_delete(file2);
	Printf("called file_delete, return %d\n", retval);

	retval = file_open(handle, "waasd");
}
