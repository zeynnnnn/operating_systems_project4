#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "simplefs.h"

int vdisk_fd; // global virtual disk file descriptor
              // will be assigned with the sfs_mount call
              // any function in this file can use this.

int size;
struct aOpenFileEntry {
    char exist;
    int mode;
    long openFilePointer;
} ;
struct aFileEntry{
    char filename[32];
    long startBlock;
    int fileLength;
    char notUsed [128-32- sizeof(long)- sizeof(int)];
} ;
// This function is simply used to a create a virtual disk
// (a simple Linux file including all zeros) of the specified size.
// You can call this function from an app to create a virtual disk.
// There are other ways of creating a virtual disk (a Linux file)
// of certain size. 
// size = 2^m Bytes
int create_vdisk (char *vdiskfilename, int m)
{
    char command[BLOCKSIZE]; 

    int num = 1;
    int count; 
    size  = num << m;
    count = size / BLOCKSIZE;
    printf ("%d %d", m, size);
    sprintf (command, "dd if=/dev/zero of=%s bs=%d count=%d",
	     vdiskfilename, BLOCKSIZE, count); 
    printf ("executing command = %s\n", command); 
    system (command); 
    return (0); 
}



// read block k from disk (virtual disk) into buffer block.
// size of the block is BLOCKSIZE.
// space for block must be allocated outside of this function.
// block numbers start from 0 in the virtual disk. 
int read_block (void *block, int k)
{
    int n;
    int offset;

    offset = k * BLOCKSIZE; 
    n = read (vdisk_fd, block, BLOCKSIZE);
    if (n != BLOCKSIZE) {
	printf ("read error\n");
	return -1;
    }
    return (0); 
}

// write block k into the virtual disk. 
int write_block (void *block, int k)
{
    int n;
    int offset;
    offset = k * BLOCKSIZE; 
    n = write (vdisk_fd, block, BLOCKSIZE);
    if (n != BLOCKSIZE) {
	printf ("write error\n");
	return (-1);
    }
    return 0; 
}


/**********************************************************************
   The following functions are to be called by applications directly. 
***********************************************************************/

int sfs_format (char *vdiskname)
{
    int mainFd = open(vdiskname,O_WRONLY | O_APPEND);
    if(mainFd < 0)
    {
        printf ("Error source:1  \n");
        return -1;
    }

    int arr[1024/ sizeof(int)];
    memset(arr, -1,1024/ sizeof(int));
    //Super Block init
   int writenByteCount= write(mainFd,&size, sizeof(size));
    if(writenByteCount <= 0)
    {  printf ("Error source:2  \n");
        return -1;
    }

    struct aOpenFileEntry emptyOpenFile;
    emptyOpenFile.exist="N";
    emptyOpenFile.mode=" ";
    emptyOpenFile.openFilePointer=NULL;
    for (int i = 0; i < 10; i++) {
        writenByteCount=  write(mainFd,&emptyOpenFile, sizeof(emptyOpenFile));
        if(writenByteCount <= 0)
        {  printf ("Error source:3  \n");
            return -1;
        }
    }
    writenByteCount=  write(mainFd,arr, 1024- sizeof(size)-10* sizeof(emptyOpenFile));
    if(writenByteCount <= 0)
        return -1;

    //file directory entries init
    struct aFileEntry emptyEntry= {0,NULL,"",""};
  /*  emptyEntry.fileLength=0;
    emptyEntry.startBlock=NULL;
    emptyEntry.filename="";
    memcpy(emptyEntry.filename,    "", 0);*/
    for (int t=0;t<7*8;t++){
        writenByteCount=  write(mainFd,&emptyEntry, sizeof(struct aFileEntry));
        if(writenByteCount <= 0)
        {  printf ("Error source:4  \n");
            return -1;
        }
    }

    // FAT init
    for(int k=0;k<1024*128;k++){
        long empS = NULL;
        writenByteCount=  write(mainFd,&empS, sizeof(long));
        if(writenByteCount <= 0)
        {  printf ("Error source:5  \n");
            return -1;
        }
    }

    if(-1==close(mainFd))
        return -1;
    return 0;
}

int sfs_mount (char *vdiskname)
{
    // simply open the Linux file vdiskname and in this
    // way make it ready to be used for other operations.
    // vdisk_fd is global; hence other function can use it. 
    vdisk_fd = open(vdiskname, O_RDWR); 
    return(0);
}

int sfs_umount ()
{
    fsync (vdisk_fd); 
    close (vdisk_fd);
    return (0); 
}


int sfs_create(char *filename)
{
    return (0);
}


int sfs_open(char *file, int mode)
{
    return (0); 
}

int sfs_close(int fd){
    return (0); 
}

int sfs_getsize (int  fd)
{
    return (0); 
}

int sfs_read(int fd, void *buf, int n){
    return (0); 
}


int sfs_append(int fd, void *buf, int n)
{
    return (0); 
}

int sfs_delete(char *filename)
{
    return (0); 
}

