#include "sfs_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fuse.h>
#include <strings.h>
#include "disk_emu.h"
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/uio.h>
#define SFSNAME		"sfs"
#define NUM_BLOCKS	1024
#define blockSize	1024
#define NumberOfInodes	99
#define ROOTDIR_INODE	0
#define MAGIC		0xABCD0005
#define MAX_FILE_SIZE	blockSize * (43)

// define the free bit map is a 1024 bit char array in the last part of the disk
char FreeBitMap[1024];

// set the block in free bit map to be free
void freeBlock( int index )
{
	FreeBitMap[index + 1] = '0';
}

// find a freebit using free bit map
int findFreeBlock()
{
	for ( int i = 0; i < 1024; i++ )
	{
		if ( FreeBitMap[i] == '0' )
		{
			FreeBitMap[i] = '1';
			return(i);
		}
	}
	return(0);
}


inode_t	InodeTable[NumberOfInodes];
file_descriptor fileDescriptorTable[NumberOfInodes];
directory_entry rootDir[NumberOfInodes];


// for getfilename
int location;

superblock_t super;

int indirectBlocks[31];


void removeInode( int i )
{
	InodeTable[i].status		= -1;

	int y = 11;
	InodeTable[i].linkCount		= -1;

	while ( y >= 0 )
	{
		InodeTable[i].data_ptrs[y] = -1;
		y--;
	}

	
	InodeTable[i].indirectPointer	= -1;

	InodeTable[i].size			= -1;
}


int AllocateInode()
{
	int i = 1;
	while ( i < NumberOfInodes )
	{
		if ( InodeTable[i].status == -1 )
		{
			InodeTable[i].status = 1;
			return(i);
		}
		i++;
	}
	return(-1);
}


// find the first free place in directory entry
int findInRoot()
{
	int i = 0;
	while ( i < NumberOfInodes )
	{
		if ( rootDir[i].num == -1 )
		{
			return(i);
		}
		i++;
	}
	return(-1);
}


void mksfs( int fresh )
{
	if ( fresh == 1 )
	{
		location = 0;
		init_fresh_disk( SFSNAME, blockSize, NUM_BLOCKS );

		for ( int i = 0; i < 1024; i++ )
		{
			FreeBitMap[i] = '0';
		}

		int j = (NumberOfInodes - 1);
		while ( j >= 0 )
		{
			fileDescriptorTable[j].inodeIndex = -1;
			removeInode( j );
			j--;
		}

		for ( int i = 0; i < NumberOfInodes; i++ )
		{
			rootDir[i].num = -1;
			strcpy( rootDir[i].name, "\0" );
		}

		int inodeBlockNumber = (sizeof(InodeTable) / blockSize)+1;


		int dirBlockNumber = (sizeof(rootDir) / blockSize)+1;



		if ( 1 == 1 )
		{
			InodeTable[0].status		= 1;
			InodeTable[0].linkCount		= dirBlockNumber;
			InodeTable[0].size			= -1;
			InodeTable[0].indirectPointer	= -1;
			int j = 11;
			while ( j >= 0 )
			{
				InodeTable[0].data_ptrs[j] = j + inodeBlockNumber + 1;
				j--;
			}
		}



		super.magicNum		= MAGIC;
		super.block_size	= blockSize;
		super.fileSystemSize		= NUM_BLOCKS;
		super.inodeTableLength	= NumberOfInodes;


		super.rootDirInode	= ROOTDIR_INODE;

// write super at block 0
		write_blocks( 0, 1, &super );

// write inodes from block 1
		write_blocks( 1, inodeBlockNumber, &InodeTable );


// Write rootDir to disk 
		write_blocks( (inodeBlockNumber + 1), dirBlockNumber, &rootDir );
	


// Write free bit map into disk, and I just left some space behind for safety 
		write_blocks( 1021, 1, &FreeBitMap );

	} else{

		int j = NumberOfInodes - 1;
		while ( j >= 0 )
		{
			fileDescriptorTable[j].inodeIndex = -1;
			j--;
		}

		init_disk( SFSNAME, blockSize, NUM_BLOCKS );

	
// read data from disk
		
		read_blocks( 0, 1, &super );

		int inodeBlockNumber = (sizeof(InodeTable) / blockSize)+1;


		int dirBlockNumber = (sizeof(rootDir) / blockSize)+1;
	


		read_blocks( 1, inodeBlockNumber, &InodeTable );


	
		read_blocks( 1021, 1, &FreeBitMap);


		
		read_blocks( (inodeBlockNumber + 1), InodeTable[ROOTDIR_INODE].linkCount, &rootDir );


		location = 0;
	}
}


int sfs_get_next_filename( char *fname )
{
	if ( location < NumberOfInodes )
	{
		while ( rootDir[location].num == -1 )
		{
			if ( location >= NumberOfInodes )
			{
				location = 0;
				return(0);
			}
			location++;
		}
		strcpy( fname, rootDir[location].name );
		location++;
		return(location);
	} else {
		location = 0;
		return(0);
	}
}


int sfs_GetFileSize( const char* path )
{
	int INODENUMBER = -1;

	int i = 0;
	while ( i < NumberOfInodes )
	{
		if ( rootDir[i].num != -1 )
		{
			if ( strcmp( rootDir[i].name, path ) == 0 )
			{
				INODENUMBER = rootDir[i].num;
			}
		}
		i++;
	}


	if ( INODENUMBER != -1 )
	{
		return(InodeTable[INODENUMBER].size);
	}

	// if path not exist
	if ( INODENUMBER == -1 )
	{
		return(-1);
	}
}


int sfs_fclose( int fileID )
{

	if ( fileDescriptorTable[fileID].inodeIndex != -1 ) {
		fileDescriptorTable[fileID].inodeIndex = -1;
		return(0);
	}

	return(-1);

}


int sfs_fwseek( int fileID, int loc )
{
	if ( InodeTable[fileDescriptorTable[fileID].inodeIndex].size >= loc )
	{
		fileDescriptorTable[fileID].wptr = loc;
	    return(0);

	}
	return(-1);


}


int sfs_frseek( int fileID, int loc )
{
	if ( InodeTable[fileDescriptorTable[fileID].inodeIndex].size >= loc )
	{
		fileDescriptorTable[fileID].rptr = loc;
	    return(0);
	}
	return(-1);


}


int sfs_fopen( char *name )
{

	// checking for edge cases
	if ( strlen( name ) > MAXFILENAME )
	{
		return(-1);
	}
	if ( name == NULL || *name == '\0' )
		return(-1);

	int newDir = -1;
	if ( 1 == 1 )
	{
		int i = 0;
		while ( i < NumberOfInodes )
		{
			if ( fileDescriptorTable[i].inodeIndex == -1 )
			{
				newDir = i;
			}
			i++;
		}
	}

	if ( newDir == -1 ) {
		return(-1);
	}  // if there is a space to open

	if ( newDir != -1 )
	{
		int fileInode = -1;

		if ( 1 == 1 )
		{
			int i = 0;
			while ( i < NumberOfInodes )
			{
				if ( rootDir[i].num != -1 )
				{
					if ( strcmp( rootDir[i].name, name ) == 0 )
					{
						fileInode = rootDir[i].num;
					}
				}
				i++;
			}
		}

		if ( fileInode != -1 )
		{
			int i = 0;


			while ( i < NumberOfInodes )
			{
				if ( fileDescriptorTable[i].inodeIndex == fileInode ) // check if file already opened
				{
					return(-1);
				}
				i++;
			}

// set the data for the file descriptor
			fileDescriptorTable[newDir].rptr		= 0;
			fileDescriptorTable[newDir].wptr		= InodeTable[fileInode].size;
			fileDescriptorTable[newDir].inodeIndex	= fileInode;


			fileDescriptorTable[newDir].inode = &(InodeTable[fileInode]);


			return(newDir);
		} else {
			// if need to create a file
			int newInodeIndex = AllocateInode();
			if ( newInodeIndex == -1 )
			{
				return(-1);
			} // if there is a free inode


			fileDescriptorTable[newDir].inodeIndex = newInodeIndex;

			int newDirEntryIndex = findInRoot();
			if ( newDirEntryIndex == -1 )
			{
				return(-1);
			} // if there is a free space in root directory


			rootDir[newDirEntryIndex].num = newInodeIndex;
			strcpy( rootDir[newDirEntryIndex].name, name );


			int newDataPtr = findFreeBlock();
			if ( newDataPtr == 0 )
			{
				return(-1);
			}


			if ( 1 == 1 )
			{
				// set the inode
				InodeTable[newInodeIndex].status		= 1;
				InodeTable[newInodeIndex].linkCount		= 1;
				InodeTable[newInodeIndex].size		= 0;
				InodeTable[newInodeIndex].indirectPointer	= -1;
				int j = 11;
				while ( j >= 1 )
				{
					InodeTable[newInodeIndex].data_ptrs[j] = -1;
					j--;
				}
				InodeTable[newInodeIndex].data_ptrs[0] = newDataPtr;
			}

// set the file descriptor
			fileDescriptorTable[newDir].inode	= &(InodeTable[newInodeIndex]);
			fileDescriptorTable[newDir].rptr	= 0;
			fileDescriptorTable[newDir].wptr	= InodeTable[newInodeIndex].size;


			int inodeBlockNumber = (sizeof(InodeTable) / blockSize)+1;


			int dirBlockNumber = (sizeof(rootDir) / blockSize)+1;

			InodeTable[ROOTDIR_INODE].size += 1;

	
// update on disk data
			write_blocks( (inodeBlockNumber + 1), dirBlockNumber, &rootDir );
	


			write_blocks( 1, inodeBlockNumber, &InodeTable );


			write_blocks( 1021, 1, &FreeBitMap );
			return(newDir);
		}
	} 
}


int sfs_fread( int fileID, char *buf, int length )
{
	int	readSize;
	int	end;
	if (fileID < 0) {
		return -1;
	}
    int	ReadInode = fileDescriptorTable[fileID].inodeIndex;
	if ( (fileID > 100) )
	{
		return(-1);
	} else {
		if ( ReadInode == -1 )
		{
			return(-1);
		} else {
			if ( InodeTable[ReadInode].size <= 0 )
			{
				return(0);
			} else{ if ( length <= 0 )
				{
					return(0);
				}
			}
		} //check for the edge condition
	}

	int begin = fileDescriptorTable[fileID].rptr / blockSize;
	if ( InodeTable[ReadInode].size >= (fileDescriptorTable[fileID].rptr + length) )
	{
		readSize	= length;
		end	= (fileDescriptorTable[fileID].rptr + length) / blockSize+1;
	}else {
			readSize	= InodeTable[ReadInode].size - fileDescriptorTable[fileID].rptr;
			end	= (InodeTable[ReadInode].size / blockSize) + 1;
		}


// check if there is a indirect block

	if ( InodeTable[ReadInode].linkCount > 12 )
	{
		read_blocks( InodeTable[ReadInode].indirectPointer, 1, &indirectBlocks );
	}

	int beginingByte = fileDescriptorTable[fileID].rptr % blockSize;

// read into a buffer
	void *buffers = malloc( blockSize * end );

	int i = begin;

// start to read

	while ( (i < InodeTable[ReadInode].linkCount) && (i < end) )
	{
		if ( i >= 12 )
		{
			read_blocks( indirectBlocks[i - 12], 1, (buffers + (i - begin) * blockSize) );
		}
		if ( i < 12 )
		{
			read_blocks( InodeTable[ReadInode].data_ptrs[i], 1, (buffers + (i - begin) * blockSize) );
		}
		i++;
	}

	if ( buf != NULL )
	{
		memcpy( buf, (buffers + beginingByte), readSize );
	}

	fileDescriptorTable[fileID].rptr += readSize;

	free( buffers );

	return(readSize);
}


int sfs_fwrite( int fileID, const char *buf, int length )
{
	if ( buf == NULL || length < 0 )
	{
		return(-1);
	}
	int	writeSize = length;

	if ( (fileID < 0) || (fileID > 100) )
	{
		return(-1);
	}
	int	writeInode = fileDescriptorTable[fileID].inodeIndex;

	if ( writeInode == -1 )
	{
		return(-1);
	}
// check for edge case

	int bytesNeeded = fileDescriptorTable[fileID].wptr + length;

	if (length==0){ return 0; } // if nothing to read

	int currentBlocks = InodeTable[writeInode].linkCount;

	if ( bytesNeeded > MAX_FILE_SIZE )
	{
		bytesNeeded	= MAX_FILE_SIZE;
		writeSize		= MAX_FILE_SIZE - fileDescriptorTable[fileID].wptr;
	}

	int blocksNeeded = (bytesNeeded / blockSize)+1;


	int moreBlock = blocksNeeded - currentBlocks;

	if ( InodeTable[writeInode].linkCount > 12 )
	{
		read_blocks( InodeTable[writeInode].indirectPointer, 1, &indirectBlocks );
	} else if ( InodeTable[writeInode].linkCount + moreBlock > 12 )  // if need to create a indirect pointer
	{
		int data_ptr = findFreeBlock();
		if ( data_ptr == 0 )
		{
			return(-1);
		} 
		if ( data_ptr != 0 ) {
			InodeTable[writeInode].indirectPointer = data_ptr;
		}
	}


	if ( moreBlock > 0 )
	{
		int i = InodeTable[writeInode].linkCount;

// address how many blocks need to be created in order to write all data
		while ( i < (InodeTable[writeInode].linkCount + moreBlock) )
		{
			int new_datablock = findFreeBlock();
			if ( new_datablock == 0 )
			{
				return(-1);
			} else {
				if ( i >= 12 )
				{
					indirectBlocks[i - 12] = new_datablock;
				}
				if ( i < 12 )
				{
					InodeTable[writeInode].data_ptrs[i] = new_datablock;
				}
			}
			i++;
		}
	}

	// update size

	if ( InodeTable[writeInode].size < bytesNeeded )
	{
		InodeTable[writeInode].size = bytesNeeded;
	}


	int	begin		= fileDescriptorTable[fileID].wptr / blockSize;

	int end = blocksNeeded;

// start tp read and then write
	void	*buffers	= malloc( blockSize * blocksNeeded );
	int	i		= begin;

	while ( (i < InodeTable[writeInode].linkCount) && (i < end) )
	{
		if ( i >= 12 )
		{
			read_blocks( indirectBlocks[i - 12], 1, (buffers + (i - begin) * blockSize) );
		}
		if ( i < 12 )
		{
			read_blocks( InodeTable[writeInode].data_ptrs[i], 1, (buffers + (i - begin) * blockSize) );
		}
		i++;
	}

	int	beginingByte	= fileDescriptorTable[fileID].wptr % blockSize;

	memcpy( (buffers + beginingByte), buf, writeSize );


	i = begin;
	while ( i < end )
	{
		if ( i >= 12 )
		{
			write_blocks( indirectBlocks[i - 12], 1, (buffers + (i - begin) * blockSize) );
		}
		if ( i < 12 )
		{
			write_blocks( InodeTable[writeInode].data_ptrs[i], 1, (buffers + ( (i - begin) * blockSize) ) );
		}
		i++;
	}
		
	InodeTable[writeInode].linkCount	+= moreBlock;

// update the on disk data

	if ( InodeTable[writeInode].linkCount > 12 )
	{
		write_blocks( InodeTable[writeInode].indirectPointer, 1, &indirectBlocks );
	}


	int inodeBlockNumber = (sizeof(InodeTable) / blockSize)+1;
	

	write_blocks( 1, inodeBlockNumber, &InodeTable );


	write_blocks( 1021, 1, &FreeBitMap );

	free( buffers );

	fileDescriptorTable[fileID].wptr = bytesNeeded;

	return(writeSize);
}


int sfs_remove( char *file )
{
	if ( file == NULL || *file == '\0' )
		return(-1);

	int fileInode = -1;
	if ( 1 == 1 )
	{
		int i = 0;
		while ( i < NumberOfInodes )
		{
			if ( rootDir[i].num != -1 )
			{
				if ( strcmp( rootDir[i].name, file ) == 0 )
				{
					fileInode = rootDir[i].num;
				}
			}
			i++;
		}
	}  // get the inode number in root directory

	if ( fileInode > 0 )
	{
		int i = 0;

		while ( (i < InodeTable[fileInode].linkCount) && (i < 12) )
		{
			freeBlock( InodeTable[fileInode].data_ptrs[i] );
			i++;
		}

		// free direct blocks in fbm


		if ( InodeTable[fileInode].linkCount > 12 )
		{
		
			read_blocks( InodeTable[fileInode].indirectPointer, 1, &indirectBlocks );

			int i = 12;

			while ( i < InodeTable[fileInode].linkCount )
			{
				freeBlock( indirectBlocks[i - 12] );
				i++;

				// free indirect blocks in fbm
			}

	
		}


		removeInode( fileInode );
		i = 0;

		while ( i < NumberOfInodes )
		{
			if ( strcmp( rootDir[i].name, file ) == 0 )
			{
				rootDir[i].num = -1;
				strcpy( rootDir[i].name, "\0" );
				break;
			}
			i++;
		}

		// remove in root directory

		InodeTable[ROOTDIR_INODE].size -= 1;


		int dirBlockNumber = (sizeof(rootDir) / blockSize)+1;
	

		int inodeBlockNumber = (sizeof(InodeTable) / blockSize)+1;
	
// update on disk data

		write_blocks( (inodeBlockNumber + 1), dirBlockNumber, &rootDir );


		write_blocks( 1, inodeBlockNumber, &InodeTable );

		write_blocks( 1021, 1, &FreeBitMap );

		return(0);
	} else {
		return(-1);
	}
}



