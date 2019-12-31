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
   // *(int*)superblock= size;
     aOpenFileEntry emptyOpenFile = {'N',-1,-1.0};
    long k =-1.0;
    emptyOpenFile.openFilePointer= k;
   // printf("exist: %c \n",emptyOpenFile.exist);
    for (int i = 0; i < 10; i++) {
        *( aOpenFileEntry*)(superblock+ i* sizeof( aOpenFileEntry))=  emptyOpenFile;
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
        long k =-1.0;
        directoryBlock[u].startBlock=k;
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
        if(write_block(directoryBlock,t+1) != 0)
        {  printf ("Error source:4  \n");
            return -1;
        }
    }


    // FAT init
    long aFatBlock [BLOCKSIZE/8];
    for (int u=0;u<(BLOCKSIZE/8);u++)
    {
        aFatBlock[u]=-1.0;
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
       iter->exist='F';
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
            if(strcmp(iter.filename,filename))
                break;
        }
        if(y< (BLOCKSIZE/ sizeof(aFileEntry)))
            break;
    }
    printf("i:%d y:%d",i,y);
    if (i>=7)
        return -1;
    long actualFileInfoLocation ;
    actualFileInfoLocation=(BLOCKSIZE/ sizeof(aFileEntry))*i+y;
    char superblock[BLOCKSIZE];
    if (read_block(&superblock,0)==-1)
        return -1;
    int k=-1;

    for (k =0;k<10;k++)
    {
        aOpenFileEntry* iter= (aOpenFileEntry*) (superblock+ k* sizeof(aOpenFileEntry))  ;
        printf("Char at the beginning of iter :%c \n",iter->exist);
        if (iter->exist=='N') {

            iter->exist='F';
            iter->mode=mode;
            iter->openFilePointer=actualFileInfoLocation;

            aOpenFileEntry* iter= (aOpenFileEntry*) (superblock+ k* sizeof(aOpenFileEntry))  ;
            printf("After updated Char at the beginning of iter :%c \nMode: %d \n Actual Pointer :%lu",iter->exist,iter->mode,iter->openFilePointer);
           if( -1==write_block(superblock,0))
               return -1;
            return k;
        }
    }
    return (-1);
}

int sfs_close(int fd){
    char superblock[BLOCKSIZE];
    if (read_block(&superblock,0)==-1)
        return -1;
        aOpenFileEntry* iter= (aOpenFileEntry*) (superblock+ fd* sizeof(aOpenFileEntry))  ;
        printf("Char at the beginning of iter :%c \n",iter->exist);
        if (iter->exist=='F') {
            iter->exist='N';
            iter->mode=-1;
            iter->openFilePointer=-1.0;
            printf("After updated Char at the beginning of iter :%c \nMode: %d \n Actual Pointer :%lu",iter->exist,iter->mode,iter->openFilePointer);
            if( -1==write_block(superblock,0))
                return -1;
            return 0;
        }

    return (-1);
}

int sfs_getsize (int  fd)
{
    char superblock[BLOCKSIZE];
    if (read_block(&superblock,0)==-1)
        return -1;
    aOpenFileEntry* iter= (aOpenFileEntry*) (superblock+ fd* sizeof(aOpenFileEntry))  ;
    printf("Char at the beginning of iter :%c \n",iter->exist);
    if (iter->exist=='F') {
       if( iter->openFilePointer!=-1.0)
       {
           int l=-99;
           long st;
           char exist;
           readFileInfos(iter->openFilePointer,&l,&st,&exist);
           return l;
       }
    }
    return (-1);
}

int sfs_read(int fd, void *buf, int n){

    char superblock[BLOCKSIZE];
    int result =read_block(&superblock,0);
    if (result !=0)
        return -1;
    aOpenFileEntry * iter= (aOpenFileEntry*) (superblock+ fd* sizeof(aOpenFileEntry) ) ;
  //  printf("Char at the beginning of iter :%c \n",iter->exist);
    if (iter->exist=='F') {
        if( iter->openFilePointer!=-1.0){
            long startPointer;
            int length =-1;
            char exist;
            readFileInfos(iter->openFilePointer,&length,&startPointer,&exist);
            if(length<n)
            {
                printf("n is bigger than File Size!");
                return -1;
            }
            double value=length;
            double divider=BLOCKSIZE;
            double result =value/divider;
           double maxIndex2 =ceil(result);
           int maxIndex=maxIndex2;
            double number =(double)n;

            result =number/divider;
            double wantedByteIndex2= ceil(result );
           int wantedByteIndex=wantedByteIndex2;
           int t=0;
           int counter=0;
           while( (startPointer!=-1.0)&&(counter<=maxIndex)) {

               counter++;
               int dataBlockNumber = startPointer +1024+ 8;
               if (counter  == wantedByteIndex) {
                   char lastBlock[BLOCKSIZE];
                   read_block(&lastBlock, dataBlockNumber);
                   for (t = 0; t < (n % BLOCKSIZE); t++) {
                       char* pointerIterInBuf=((char *) buf + t + ((counter - 1) * BLOCKSIZE));
                       *(pointerIterInBuf) = lastBlock[t];
                   }
                   printf(" after change: %s", (char*)buf );
                   break;
               }
               printf("BURDAAAA: %c",*((char *) buf + BLOCKSIZE * counter));
              read_block(((char *) buf + BLOCKSIZE * counter), dataBlockNumber);
               printf("sONRA:%c",*((char *) buf + BLOCKSIZE * counter));

               startPointer= findNextBlockFromFat( startPointer);

           }
            return ( counter-1)*BLOCKSIZE+t;
        }
        else
            return -1;
    }
    printf("FD is not valid  to read!");
    return (-1);

}
long  nextEmptyFinder(){
    long starterBlocks[56];
    for (int j = 0; j <56 ; j++) {
        starterBlocks[j]=-1.0;
    }
    int length=-3;
    long start;
    char exist;
    int counter =0;
    for (int i = 0; i < 56; ++i) {
        readFileInfos(i,&length,&start,&exist);
        if (exist=='F')
        {
            starterBlocks[counter]=start;
            counter++;
        }

    }

    long FatBlock [BLOCKSIZE/ sizeof(long)];
    for (int u=0;u<(BLOCKSIZE/ sizeof(long));u++)
    {
        long m =-1.0;
        FatBlock[u]=m;
    }
    for(int k=0;k<1024;k++) {
        if (read_block(&FatBlock, 8 + k) < 0) {
            return -1;
        } else
        {
            for(int m=0;m<(BLOCKSIZE/ sizeof(long));m++)
            {
               if( FatBlock[m] ==-1.0 )
               {
                   int boolForBlockNumIsTaken=0;
                   long num =(k*(BLOCKSIZE/ sizeof(long)))+m;
                   for (int t=0;t<counter;t++)
                       if(num==starterBlocks[t])
                       {
                            boolForBlockNumIsTaken=1;
                           break;
                       }

                if(boolForBlockNumIsTaken==0)
                   return num;
               }
            }

        }
    }
    return -1;
}

int sfs_append(int fd, void *buf, int n)
{
    char superblock[BLOCKSIZE];
    if (read_block(&superblock,0)==-1)
        return -1;
    aOpenFileEntry* iter= (aOpenFileEntry*) (superblock+ fd* sizeof(aOpenFileEntry))  ;

    if (iter->exist=='F') {
        if (iter->mode==MODE_APPEND){
        printf("fd's aopenfileentry status :%c \n",iter->exist);
        if( iter->openFilePointer!=-1.0){
            long startBlockNo;
            int length ;
            char exist;
            readFileInfos(iter->openFilePointer,&length,&startBlockNo,&exist);

            // if there were no startblock at the beginning
            if( (startBlockNo==-1.0)&&(n>0))
            {
                long emptyFATEntry=  nextEmptyFinder();
                updateFileInfos(iter->openFilePointer,0,emptyFATEntry);
                startBlockNo =emptyFATEntry;
            }
            printf("fd's lenght :%d \n\n",length);
            printf("fd's aopenfileentry startblock :%lu \n",startBlockNo);
            int iterLen =length;
          //  long lastNotNullStartBlockNo =NULL;
            long fatBlockfromRead[BLOCKSIZE/ sizeof(long)];
            //iterate used blocks
            while ((startBlockNo!=-1.0)&& (iterLen>BLOCKSIZE)) {
                iterLen=iterLen-BLOCKSIZE;
                startBlockNo= findNextBlockFromFat( startBlockNo);
            }

            printf("fd's aopenfileentry startblock :%lu \n",startBlockNo);


            int notWrittenByteNumber=n;
//readFileInfos(iter->openFilePointer,&length,&startBlockNo);
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
               long emptyFATEntry= nextEmptyFinder();
               if(-1== emptyFATEntry)
                   return -1;
               updateFatInfos(startBlockNo,emptyFATEntry);
               startBlockNo= emptyFATEntry;


               void *dataBlockNumber = startBlockNo+8+1024;
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
            long k =-1.0;
           updateFileInfos(iter->openFilePointer,n,k);


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
void updateFatInfos(long blockno,long newValue){
    long fatBlockfromRead[BLOCKSIZE/ sizeof(long)];
    int aimedblockno= 8+blockno/(BLOCKSIZE/ sizeof(long));
    read_block(&fatBlockfromRead,aimedblockno);
    fatBlockfromRead[ (blockno%(BLOCKSIZE/ sizeof(long)))]=newValue;
    write_block(fatBlockfromRead,aimedblockno);
}

void updateFileInfos(int blockno,int length,long startnewBlock)
{
    aFileEntry blackfat[BLOCKSIZE/ sizeof(aFileEntry)];
    int aimedblockno= blockno/(BLOCKSIZE/ sizeof(aFileEntry));
    read_block(&blackfat,aimedblockno+1);
    if(startnewBlock!=-1.0){
       printf("girdi");
        ( blackfat[ (blockno%(BLOCKSIZE/ sizeof(aFileEntry)))]).startBlock=startnewBlock;
    }

    (blackfat[ (blockno)%(BLOCKSIZE/ sizeof(aFileEntry))]).fileLength+=length;
    write_block(blackfat,1+blockno/(BLOCKSIZE/ sizeof(aFileEntry)));

}
void readFileInfos(int blockno,int* length,long *startnewBlock, char*exists)
{
    aFileEntry blackfat[BLOCKSIZE/ sizeof(aFileEntry)];
    int aimedblockno= blockno/(BLOCKSIZE/ sizeof(aFileEntry));
    read_block(&blackfat,aimedblockno+1);
    aFileEntry aFileEntry1=(blackfat[ (blockno)%(BLOCKSIZE/ sizeof(aFileEntry))]);
    *startnewBlock=aFileEntry1.startBlock;
    *length= aFileEntry1.fileLength;
    *exists =aFileEntry1.exist;

}
long findNextBlockFromFat(long startBlock){
    long fatBlockfromRead[BLOCKSIZE/ sizeof(long)];
    int aimedblockno= 8+startBlock/(BLOCKSIZE/ sizeof(long));
    read_block(&fatBlockfromRead,aimedblockno);
    long nextBlock =fatBlockfromRead[ startBlock%(BLOCKSIZE/ sizeof(long))];
    return nextBlock;
}



int sfs_delete(char *filename)
{
    return (0); 
}
