#ifndef _INCLUDE_SFS_API_H_
#define _INCLUDE_SFS_API_H_

#include <stdint.h>

#define MAXFILENAME 20

// superblock, inodetable and directory entries (pointing by the root directory) are on disk, but file descriptor table is only in memory

typedef struct superblock_t{
    int magicNum;
    int block_size;
    int fileSystemSize;
    int inodeTableLength;
    int rootDirInode;
} superblock_t;

typedef struct inode_t {
    // to show if the inode has been taken or not
    int status;
    int linkCount;
    int size;
    int data_ptrs[12];
    int indirectPointer; 
    // since the others are useless in this assignment, I ignores them
} inode_t;


typedef struct file_descriptor {
    int inodeIndex;
    inode_t* inode; 
    int wptr;
    int rptr;
} file_descriptor;


typedef struct directory_entry{
    int num; 
    char name[21];  
}directory_entry;


void mksfs(int fresh);
int sfs_get_next_filename(char *fname);
int sfs_GetFileSize(const char* path);
int sfs_fopen(char *name);
int sfs_fclose(int fileID);
int sfs_fread(int fileID, char *buf, int length);
int sfs_fwrite(int fileID, const char *buf, int length);
int sfs_fwseek(int fileID, int loc);
int sfs_frseek(int fileID, int loc);
int sfs_remove(char *file);

#endif 
