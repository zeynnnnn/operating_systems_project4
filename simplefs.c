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

typedef struct  {
    char exist;
    int mode;
    long openFilePointer;
} aOpenFileEntry;

typedef struct {
    int fileLength;
    long startBlock;
    char exist;
    char filename[32];
    char notUsed [78];
}aFileEntry ;

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
    lseek(vdisk_fd, (off_t) offset, SEEK_SET);
    n = read (vdisk_fd, block, BLOCKSIZE);
    printf("n:%d\n",n) ;
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
    lseek(vdisk_fd, (off_t) offset, SEEK_SET);
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

    int mainFd = open(vdiskname,O_RDWR );
    vdisk_fd = mainFd;
    if(mainFd < 0)
    {
        printf ("Error source:1  \n");
        return -1;
    }

    //Super Block init

    char superblock[BLOCKSIZE];
    for (int j = 0; j < BLOCKSIZE; ++j) {
        superblock[j]='k';
    }
   // *(int*)superblock= size;
     aOpenFileEntry emptyOpenFile = {'N',-1,NULL};
   // printf("exist: %c \n",emptyOpenFile.exist);
    for (int i = 0; i < 10; i++) {
        *( aOpenFileEntry*)(superblock+ i* sizeof( aOpenFileEntry)+ sizeof(int))=  emptyOpenFile;
     //  printf("Struct in the will be written:%d\n\n" ,( *( aOpenFileEntry*)(superblock+i* sizeof( aOpenFileEntry)+ sizeof(int))).mode);
    }
 /*   for (int j = 0; j < BLOCKSIZE; j++) {
        printf("%c",superblock[j]);
    }*/
   int writenByteCount= write_block(superblock,0);
    if(writenByteCount < 0)
    {
        printf ("Error source:2  \n");
        return -1;
    }
    char written[BLOCKSIZE] ;
    for (int j = 0; j < BLOCKSIZE; j++) {
        written[j]='O';
    }
   /* printf("Written From file: %s \n", written);
    printf("Written From file: %d \n", sizeof(written));*/
    /*  if (read_block(written,0)==0) // read succesfull                            ///test
      {
          aOpenFileEntry * wr ;
          wr =((char*)written+1);
         //for (int j = 0; j < BLOCKSIZE; j++) {
         //   printf("%c",written[j]);
         //}
   //    printf("Written From file: %d \n", sizeof(written));
    for (int j = 0; j < 10;j++) {
        aOpenFileEntry * wr ;
        wr =((char*)written+j* sizeof( aOpenFileEntry)+ sizeof(int));
           //  aOpenFileEntry wr =*( aOpenFileEntry*)(written[1+j* sizeof( aOpenFileEntry)]);
            printf("WrittenFrom file: %c\n",wr->exist);
        }
    }
    else
        printf("boj");*/
    //file directory entries init
     aFileEntry directoryBlock [BLOCKSIZE/128];
         //   ={ {  .fileLength = -1, .startBlock = NULL, .exist = 'N', .filename="",.notUsed="" } };
    for (int u=0;u<BLOCKSIZE/128;u++)
    {
        directoryBlock[u].exist='N';
        strcpy( directoryBlock[u].filename,"");
        directoryBlock[u].startBlock=NULL;
        directoryBlock[u].fileLength =-1;
        strcpy( directoryBlock[u].notUsed,"");
    }
    printf("%c\n",directoryBlock[4].exist);
    /*  emptyEntry.fileLength=0;
      emptyEntry.startBlock=NULL;
      emptyEntry.filename="";
      memcpy(emptyEntry.filename,    "", 0);*/
    for (int t=0;t<7;t++){
        //   writenByteCount=  write(mainFd,&emptyEntry, sizeof( aFileEntry));
        if(write_block(directoryBlock,t+1) < 0)
        {  printf ("Error source:4  \n");
            return -1;
        }
    }


    // FAT init
    long aFatBlock [BLOCKSIZE/8];
    for (int u=0;u<(BLOCKSIZE/8);u++)
    {
        aFatBlock[u]=NULL;
    }
    for(int k=0;k<1024;k++){
        if(write_block(aFatBlock,8+k) < 0)
        {  printf ("Error source:5  \n");
            return -1;
        }
    }

    vdisk_fd =-1;
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
{   //printf("afileentrysize:%d", sizeof(aFileEntry));
     aFileEntry fileBlocks[7][BLOCKSIZE/ sizeof( aFileEntry)];
     for (int k =1;k<8;k++)
     {
         read_block(&fileBlocks[k-1],k);
     }

    int i=-1;
    int y=-1;

    for (i = 0; i < 7; i++) {
           for (y =0; y<(BLOCKSIZE/ sizeof( aFileEntry));y++){
                aFileEntry iter = fileBlocks[i][y];
               if(iter.exist=='N')
                   break;
           }
           if(y< (BLOCKSIZE/ sizeof(aFileEntry)))
               break;
       }
       printf("i:%d y:%d",i,y);
        aFileEntry * iter ;
        iter=& fileBlocks[i][y];
       strcpy ( iter->filename,filename);
       iter->exist='Y';
       iter->fileLength=0;
       write_block(fileBlocks[i],i+1); //1 comes from superblock

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

