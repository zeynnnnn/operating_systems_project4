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
    int openFilePointer;
    int lastreadLocation;
} aOpenFileEntry;

typedef struct {
    int fileLength;
    int startBlock;
    char exist;
    char filename[32];
    char notUsed [86];
}aFileEntry ;

typedef struct {
    int nextFat;
    char used;
}aFatEntry;

aOpenFileEntry openFileBlock[10];

// This function is simply used to a create a virtual disk
// (a simple Linux file including all zeros) of the specified size.
// You can call this function from an app to create a virtual disk.
// There are other ways of creating a virtual disk (a Linux file)
// of certain size. 
// size = 2^m Bytes
int create_vdisk (char *vdiskfilename, int m)
{
   // printf("SIZE OF AFATENTRY:%d,afileentry%d, aopenfileEntry%d", sizeof(aFatEntry), sizeof(aFileEntry), sizeof(aOpenFileEntry));
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
  //  printf("n:%d\n",n) ;
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
    *(int*)superblock=size;
   int writenByteCount= write_block(superblock,0);
    if(writenByteCount < 0)
    {
        printf ("Error source:2  \n");
        return -1;
    }

    //Init openfile entry table

    aOpenFileEntry emptyOpenFile = {'N',-1,-1,0};
    int k =-1;
    emptyOpenFile.openFilePointer= k;
    // printf("exist: %c \n",emptyOpenFile.exist);
    for (int i = 0; i < 10; i++) {
        openFileBlock[i] =  emptyOpenFile;
    }

    //file directory entries init
     aFileEntry directoryBlock [BLOCKSIZE/128];
         //   ={ {  .fileLength = -1, .startBlock = NULL, .exist = 'N', .filename="",.notUsed="" } };
    for (int u=0;u<BLOCKSIZE/128;u++)
    {
        directoryBlock[u].exist='N';
        strcpy( directoryBlock[u].filename,"");
        int k =-1;
        directoryBlock[u].startBlock=k;
        directoryBlock[u].fileLength =-1;
        strcpy( directoryBlock[u].notUsed,"");
    }
    printf("%c\n",directoryBlock[4].exist);

    for (int t=0;t<7;t++){
        //   writenByteCount=  write(mainFd,&emptyEntry, sizeof( aFileEntry));
        if(write_block(directoryBlock,t+1) != 0)
        {  printf ("Error source:4  \n");
            return -1;
        }
    }


    // FAT init
    //make every fat block available
    aFatEntry aFatBlock [sizeof(aFatEntry)];
    for (int u=0;u<(BLOCKSIZE/8);u++)
    {
        aFatBlock[u].used='N';
        aFatBlock[u].nextFat=-1;
    }

    for(int k=0;k<1024;k++){
        if(write_block(aFatBlock,8+k) < 0)
        {
            printf ("Error source:5  \n");
            return -1;
        }
    }

    //make bigger than disk size fat block unavailable
    int realDataBlockCount= (size/BLOCKSIZE) -1032;
    int fileSizeIsNotLargeEnoughSinceBlockNoOffset=realDataBlockCount%(BLOCKSIZE/ sizeof(aFatEntry));
    int fileSizeIsNotLargeEnoughSinceBlockNoFatBlockNo=realDataBlockCount/(BLOCKSIZE/ sizeof(aFatEntry));
    for (int u=fileSizeIsNotLargeEnoughSinceBlockNoOffset;u<(BLOCKSIZE/ sizeof(aFatEntry));u++)
    {
        aFatBlock[u].used='F';
        aFatBlock[u].nextFat=-1;
    }
    if(write_block(aFatBlock,8+fileSizeIsNotLargeEnoughSinceBlockNoFatBlockNo) < 0)
    {
        printf ("Error source:5  \n");
        return -1;
    }
    for (int u=0;u<(BLOCKSIZE/ sizeof(aFatEntry));u++)
    {
        aFatBlock[u].used='F';
        aFatBlock[u].nextFat=-1;
    }
    for(int k=fileSizeIsNotLargeEnoughSinceBlockNoFatBlockNo+1;k<1024;k++){
        if(write_block(aFatBlock,8+k) < 0)
        {
            printf ("Error source:5  \n");
            return -1;
        }
    }


    aFatEntry written[BLOCKSIZE/ sizeof(aFatEntry)] ;

    /* printf("Written From file: %s \n", written);
     printf("Written From file: %d \n", sizeof(written));*/
    for(int y =0; y<1024;y++){
        if (read_block(written,8+y)==0) // read succesfull                            ///test
        {
            printf("\nWritten From Fat block: %d \n", y);
            for (int j = 0; j <( BLOCKSIZE/ sizeof(aFatEntry)); j++) {
                printf("%c",written[j].used);
            }
        }
        else
            printf("boj");
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
{
    //printf("afileentrysize:%d", sizeof(aFileEntry));
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
  //     printf("i:%d y:%d",i,y);
        aFileEntry * iter ;
        iter=& fileBlocks[i][y];
       strcpy ( iter->filename,filename);
       iter->exist='F';
       iter->startBlock=-1;
        strcpy ( iter->notUsed,"filename");
       iter->fileLength=0;
       write_block(fileBlocks[i],i+1); //1 comes from superblock

    return (0);
}


int sfs_open(char *filename, int mode)
{
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

            if(strcmp(iter.filename,filename) == 0)
            {
                printf("\n\nOPENED:::::%s\n",iter.filename);
                break;
            }

        }
        if(y< (BLOCKSIZE/ sizeof(aFileEntry)))
            break;
    }
 //   printf("i:%d y:%d",i,y);
    if (i>=7)
        return -1;
    int actualFileInfoLocation ;
    actualFileInfoLocation=(BLOCKSIZE/ sizeof(aFileEntry))*i+y;
   /* char superblock[BLOCKSIZE];
    if (read_block(&superblock,0)==-1)
        return -1;
        */
    int k=-1;

    for (k =0;k<10;k++)
    {
        aOpenFileEntry* iter= & openFileBlock[k]  ;
     //   printf("Char at the beginning of iter :%c \n",iter->exist);
        if (iter->exist=='N') {
            iter->lastreadLocation=0;
            iter->exist='F';
            iter->mode=mode;
            iter->openFilePointer=actualFileInfoLocation;
            return k;
        }
    }
    return (-1);
}

int sfs_close(int fd){
  /*  char superblock[BLOCKSIZE];
    if (read_block(&superblock,0)==-1)
        return -1;
        */
    aOpenFileEntry* iter= & openFileBlock[fd]  ;
  //  printf("Close Called BEFORE:%c \nMode: %d \n Actual Pointer :%d",iter->exist,iter->mode,iter->openFilePointer);
        if (iter->exist=='F') {
            iter->exist='N';
            iter->mode=-1;
            iter->openFilePointer=-1;
            iter->lastreadLocation=-1;
         //   printf("Close Called AFTER:%c \nMode: %d \n Actual Pointer :%d",iter->exist,iter->mode,iter->openFilePointer);
          /*  if( -1==write_block(superblock,0))
                return -1;*/
            return 0;
        }

    return (-1);
}

int sfs_getsize (int  fd)
{
  /*  char superblock[BLOCKSIZE];
    if (read_block(&superblock,0)==-1)
        return -1;*/
    aOpenFileEntry* iter= & openFileBlock[fd]  ;

  //  printf("Char at the beginning of iter :%c \n",iter->exist);
    if (iter->exist=='F') {
       if( iter->openFilePointer!=-1)
       {
           int l=-99;
           int st;
           char exist;
           readFileInfos(iter->openFilePointer,&l,&st,&exist);
           return l;
       }
    }
    return (-1);
}

int sfs_read(int fd, void *buf, int n){

  /*  char superblock[BLOCKSIZE];
    int result =read_block(&superblock,0);
    if (result !=0)
        return -1;
        */
    aOpenFileEntry* iter= & openFileBlock[fd]  ;
 //   aOpenFileEntry * iter= (aOpenFileEntry*) (superblock+ fd* sizeof(aOpenFileEntry) ) ;
  //  printf("Char at the beginning of iter :%c \n",iter->exist);
    if (iter->exist=='F') {
        if (iter->mode!=MODE_READ)
        {
            printf("Mode is not suited for reading operation!");
            return -1;
        }
        if( iter->openFilePointer!=-1){

            int startOfRead= iter->lastreadLocation;
            int startPointer;
            int length =-1;
            char exist;
            readFileInfos(iter->openFilePointer,&length,&startPointer,&exist);
            if(length-startOfRead<n)
            {
                printf("n is bigger than File Size!");
                return -1;
            }
            //what to
            double value=length-startOfRead;
            double divider=BLOCKSIZE;
            double result =value/divider;
           double maxIndex2 =ceil(result);
           int maxIndex=maxIndex2;
            double number =(double)n;
            result =number/divider;
            double wantedByteIndex2= ceil(result );
           int wantedByteIndex=wantedByteIndex2;


           int blockToReadID= startOfRead/BLOCKSIZE;
           int blockToReadOffset= startOfRead%BLOCKSIZE;
           int t=0;
           int counter=0;
           int notReadNumber=n;
            for (int i = 0; i < blockToReadID; i++) {
                startPointer= findNextBlockFromFat( startPointer);
            }
           while( (startPointer!=-1)&&(counter<=maxIndex)) {
               counter++;
               int dataBlockNumber = startPointer +1024+ 8;
               if (counter  == 1) {
                   char firstBlock[BLOCKSIZE];
                   read_block(&firstBlock, dataBlockNumber);
                   for (t = 0; t< BLOCKSIZE- blockToReadOffset; t++) {
                       char* pointerIterInBuf=((char *) buf + t );
                       *(pointerIterInBuf) = firstBlock[t+blockToReadOffset];
                       notReadNumber--;
                   }
               } else if( (counter  == wantedByteIndex) &&notReadNumber<BLOCKSIZE){
                   char lastBlock[BLOCKSIZE];
                   read_block(&lastBlock, dataBlockNumber);
                   int earlierReadCount =n-notReadNumber;
                   for (t = 0; t < notReadNumber; t++) {
                       char* pointerIterInBuf=((char *) buf +t+earlierReadCount);
                       *(pointerIterInBuf) = lastBlock[t];
                       notReadNumber--;
                   }
                   break;
               } else
               {
                   read_block(((char *) buf + BLOCKSIZE * counter), dataBlockNumber);
                   notReadNumber=notReadNumber-BLOCKSIZE;
               }
               startPointer= findNextBlockFromFat( startPointer);

           }
           startOfRead=startOfRead+n;
            updateOpenFileInfos(fd,startOfRead);
            return ( counter-1)*BLOCKSIZE+t;
        }
        else
            return -1;
    }
    printf("FD is not valid  to read!");
    return (-1);

}
int  nextEmptyFinder(){

    aFatEntry FatBlock [BLOCKSIZE/ sizeof(aFatEntry)];
    for (int u=0;u<(BLOCKSIZE/ sizeof(aFatEntry));u++)
    {
        FatBlock[u].nextFat =-1;
        FatBlock[u].used='H';
    }
    for(int k=0;k<1024;k++) {
        if (read_block(&FatBlock, 8 + k) < 0) {
            return -1;
        } else
        {
            for(int m=0;m<(BLOCKSIZE/ sizeof(aFatEntry));m++)
            {
                aFatEntry iter =FatBlock[m];
               if( iter.used =='N' )
               {
                   int num =(k*(BLOCKSIZE/ sizeof(aFatEntry)))+m;
                   return num;
               }
            }

        }
    }
    return -1;
}

int sfs_append(int fd, void *buf, int n)
{
    /*char superblock[BLOCKSIZE];
    if (read_block(&superblock,0)==-1)
        return -1;
        */
   // aOpenFileEntry* iter= (aOpenFileEntry*) (superblock+ fd* sizeof(aOpenFileEntry))  ;
    aOpenFileEntry* iter= & openFileBlock[fd]  ;

    if (iter->exist=='F') {
        if (iter->mode==MODE_APPEND){
   //     printf("\n\nfd's aopenfileentry status :%c \n",iter->exist);
        if( iter->openFilePointer!=-1){
            int startBlockNo;
            int length ;
            char exist;
            readFileInfos(iter->openFilePointer,&length,&startBlockNo,&exist);

            // if there were no startblock at the beginning
            if( (startBlockNo==-1)&&(n>0))
            {
                int emptyFATEntry=  nextEmptyFinder();
                updateFileInfos(iter->openFilePointer,0,emptyFATEntry);
                startBlockNo =emptyFATEntry;
            }

            int iterLen =length;
            //iterate used blocks
            while ((startBlockNo!=-1)&& (iterLen>BLOCKSIZE)) {
                iterLen=iterLen-BLOCKSIZE;
                startBlockNo= findNextBlockFromFat( startBlockNo);
            }



            int notWrittenByteNumber=n;
            int t=0;
            int counter=0;
          char halfBlock[BLOCKSIZE];
            int  halfFullSize= length%BLOCKSIZE;
            if((halfFullSize!=0) ||(length==0)){
                read_block(&halfBlock, (int)startBlockNo +8 +1024);
                int y=-1;
                for(y=0;(y<BLOCKSIZE-halfFullSize)&&(notWrittenByteNumber>0);y++)
                {
                    halfBlock[halfFullSize+y]=*((char*)buf+y);
                    notWrittenByteNumber=notWrittenByteNumber-1;
                }
                write_block(halfBlock,(int)startBlockNo+8+1024);
            }

           while (notWrittenByteNumber>0)
           {
               int emptyFATEntry= nextEmptyFinder();
               if(-1== emptyFATEntry)
                   return -1;
               updateFatInfos(startBlockNo,emptyFATEntry,0);
               startBlockNo= emptyFATEntry;
               int dataBlockNumber = startBlockNo+8+1024;
               counter++;
               if (notWrittenByteNumber <=BLOCKSIZE) {
                   char lastBlock[BLOCKSIZE];
                   for (t = 0; t < (n % BLOCKSIZE); t++) {
                       lastBlock[t]=  *(char *) buf + t + (counter - 1) * BLOCKSIZE ;
                   }
                   write_block(lastBlock, dataBlockNumber);
                   break;
               }
               write_block((char *) buf + BLOCKSIZE * counter, dataBlockNumber);
               notWrittenByteNumber=notWrittenByteNumber-BLOCKSIZE;

           }

            //update file length

           updateFileInfos(iter->openFilePointer,n,-1);
            return counter*BLOCKSIZE+t;
        }
        else
            return -1;
        }
        printf("Mode is not append mode, appending is not allowed!");
        return -1;
    }
    printf("FD is not valid to append!");
    return (-1);

}
void updateFatInfos(int blockno,int newValue,int blockNoEmptied){
    aFatEntry fatBlockfromRead[BLOCKSIZE/ sizeof(aFatEntry)];
    int aimedblockno;
    if(blockNoEmptied==1)
        updateFatChar(blockno,'N');
    else
        updateFatChar(newValue,'Y');
    if(blockno!=-1){
        aimedblockno= 8+blockno/(BLOCKSIZE/ sizeof(aFatEntry));
        read_block(&fatBlockfromRead,aimedblockno);
        fatBlockfromRead[ (blockno%(BLOCKSIZE/ sizeof(aFatEntry)))].nextFat=newValue;
        write_block(fatBlockfromRead,aimedblockno);
    }

}
void updateFatChar(int blockno,char used){
    aFatEntry fatBlockfromRead[BLOCKSIZE/ sizeof(aFatEntry)];
    int aimedblockno;
    aimedblockno= 8+blockno/(BLOCKSIZE/ sizeof(aFatEntry));
    read_block(&fatBlockfromRead,aimedblockno);

    (fatBlockfromRead[ (blockno)%(BLOCKSIZE/ sizeof(aFatEntry))]).used=used;
    write_block(fatBlockfromRead,8+blockno/(BLOCKSIZE/ sizeof(aFatEntry)));
}

void updateFileInfos(int blockno,int length,int startnewBlock)
{
    aFileEntry blackfat[BLOCKSIZE/ sizeof(aFileEntry)];
    int aimedblockno= blockno/(BLOCKSIZE/ sizeof(aFileEntry));
    read_block(&blackfat,aimedblockno+1);
    if(startnewBlock!=-1){
      // printf("girdi");
        ( blackfat[ (blockno%(BLOCKSIZE/ sizeof(aFileEntry)))]).startBlock=startnewBlock;
        updateFatInfos(-1,startnewBlock,0);
    }

    (blackfat[ (blockno)%(BLOCKSIZE/ sizeof(aFileEntry))]).fileLength+=length;
    write_block(blackfat,1+blockno/(BLOCKSIZE/ sizeof(aFileEntry)));

}
void updateOpenFileInfos(int fd,int newReadStart)
{
    aOpenFileEntry* iter= & openFileBlock[fd]  ;
    iter->lastreadLocation=newReadStart;

}
void readFileInfos(int blockno,int* length,int *startnewBlock, char*exists)
{
    aFileEntry blackfat[BLOCKSIZE/ sizeof(aFileEntry)];
    int aimedblockno= blockno/(BLOCKSIZE/ sizeof(aFileEntry));
    read_block(&blackfat,aimedblockno+1);
    aFileEntry aFileEntry1=(blackfat[ (blockno)%(BLOCKSIZE/ sizeof(aFileEntry))]);
    *startnewBlock=aFileEntry1.startBlock;
    *length= aFileEntry1.fileLength;
    *exists =aFileEntry1.exist;

}
int findNextBlockFromFat(int startBlock){

    aFatEntry fatBlockfromRead[BLOCKSIZE/ sizeof(aFatEntry)];
    int aimedblockno= 8+startBlock/(BLOCKSIZE/ sizeof(aFatEntry));
    read_block(&fatBlockfromRead,aimedblockno);
    int nextBlock =fatBlockfromRead[ startBlock%(BLOCKSIZE/ sizeof(aFatEntry))].nextFat;
    return nextBlock;
}



int sfs_delete(char *filename)
{
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
            if(strcmp(iter.filename,filename) == 0)
                break;
        }
        if(y< (BLOCKSIZE/ sizeof(aFileEntry)))
            break;
    }
    printf("i:%d y:%d",i,y);
    aFileEntry * iter ;
    iter=& fileBlocks[i][y];
    iter->exist='N';
    iter->fileLength=-1;
    int iterBlock =iter->startBlock;
    int iterBlock2=-2;
    while (iterBlock2!=-1)
    {
        iterBlock2=findNextBlockFromFat(iterBlock);
        updateFatInfos(iterBlock,-1,1);
    }
    write_block(fileBlocks[i],i+1); //1 comes from superblock

    return (0);
}
