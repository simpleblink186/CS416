/*
  Simple File System

  This code is derived from function prototypes found /usr/include/fuse/fuse.h
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>
  His code is licensed under the LGPLv2.

*/

#include "params.h"
#include "block.h"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <libgen.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h> //for timestamps

#ifdef HAVE_SYS_XATTR_H
#include <sys/xattr.h>
#endif

#include "log.h"


/******************************************************************/
//Didn't know if I should put this in a .h file, we can move if needed
//#definitions
#define INODEBLOCKS 189
#define DATABLOCKS 32479
#define BLOCKSPERINODE 112

//enums and structs
typedef enum _bool{
	FALSE, TRUE
}bool;

typedef enum _filetype{
	FILE_NODE, DIR_NODE
}filetype;

//This is probably going to need more data
typedef struct _inode{	
	int size; //size of file
	int numBlocks; //how many blocks it currently takes up total
	int blockNum[BLOCKSPERINODE]; //physical block numbers (remove some of these when adding other attributes)
	int links; //links to file (should always be 1 I'm pretty sure)
 	int indirectionBlock;//which block the inode continues
 	filetype type; //FILE_NODE or DIR_NODE (for extra credit)
 	mode_t filemode; //read/write/execute
 	time_t createtime;//created 
 	time_t modifytime;//last modified
 	time_t accesstime;//last accessed
	int userId; //for permissions
	int groupId; //for permissions
	char* path; //file path
}inode;

//global vars
int inodes[INODEBLOCKS];
int datablocks[DATABLOCKS];
inode* root = NULL;

//more methods
int findInode(const char* path){
	int block = 0;
	char buffer[BLOCK_SIZE];
	while(block < INODEBLOCKS){
		block_read(block, (void*)buffer);
		if(strcmp(((inode*)buffer)->path, path) == 0){
			break;
		}
		block++;
	}
	
	if(block == INODEBLOCKS){
		return -1; //TODO: make sure this is what returns on error.
	}
	
	return block;
}

/******************************************************************/


///////////////////////////////////////////////////////////
//
// Prototypes for all these functions, and the C-style comments,
// come indirectly from /usr/include/fuse.h
//

/**
 * Initialize filesystem
 *
 * The return value will passed in the private_data field of
 * fuse_context to all file operations and as a parameter to the
 * destroy() method.
 *
 * Introduced in version 2.3
 * Changed in version 2.6
 */

void *sfs_init(struct fuse_conn_info *conn){
    fprintf(stderr, "in bb-init\n");
    log_msg("\nsfs_init()\n");

	char buffer[BLOCK_SIZE];

	struct sfs_state* state = SFS_DATA; //this stuff defined in param.h
	disk_open(SFS_DATA->diskfile);
	fprintf(stderr, "size of inode is: %d\n", sizeof(inode));
		
	//intializing arrays 
	int i = 0;
	while(i < INODEBLOCKS){
		inodes[i] = 0;
		i++;
	}
	i=0;
	while(i < DATABLOCKS){
		datablocks[i] = 0;
		i++;
	}	
	memset((void*)buffer, 0, BLOCK_SIZE);
	root = (inode*)buffer;
	root->path = "/";
	root->size = 0;
	root->numBlocks = 0;
	root->links = 1;
	root->indirectionBlock = FALSE;
	root->type = DIR_NODE;
	root->filemode = 0777;
	root->createtime = time(NULL);
	root->accesstime = root->createtime;
	root->modifytime = root->createtime;
	root->userId = getuid();
	root->groupId = getgid();

	block_write(0, (void*)root);
	inodes[0] = 1;
	log_conn(conn);
    log_fuse_context(fuse_get_context());
	log_msg("leaving init function\n");
    return state;
}

/**
 * Clean up filesystem
 *
 * Called on filesystem exit.
 *
 * Introduced in version 2.3
 */
void sfs_destroy(void *userdata){
    fprintf(stderr,"destroy");
    log_msg("\nsfs_destroy(userdata=0x%08x)\n", userdata);
}

/** Get file attributes.
 *
 * Similar to stat().  The 'st_dev' and 'st_blksize' fields are
 * ignored.  The 'st_ino' field is ignored except if the 'use_ino'
 * mount option is given.
 */
int sfs_getattr(const char *path, struct stat *statbuf){
	fprintf(stderr, "in get attr\n");
    int retstat = 0;
    
    log_msg("\nsfs_getattr(path=\"%s\", statbuf=0x%08x)\n",	  path, statbuf);
	int block = findInode(path);
	if (block == -1){
//	if(strcmp(path, "/")==0 && inodes[0] == 0){
		log_msg("if stmt of getattr\n");
		fprintf(stderr,"File not found\n");
		log_msg("File not found\n");
		return -1; //TODO: make sure this is right
	}
	else{
		fprintf(stderr,"in else\n");
		char buffer[BLOCK_SIZE];
		block_read(block, (void*)buffer);

		//fill in stat
		statbuf->st_dev = 0;
		statbuf->st_ino = 0;
		statbuf->st_rdev = 0;
		statbuf->st_mode = S_IFDIR | 0755;
		statbuf->st_nlink = 2;
		statbuf->st_uid = ((inode*)buffer)->userId;
		statbuf->st_gid = ((inode*)buffer)->groupId;
		statbuf->st_size = ((inode*)buffer)->size;
		statbuf->st_atime = ((inode*)buffer)->accesstime;
		statbuf->st_mtime = ((inode*)buffer)->modifytime;
		statbuf->st_ctime = ((inode*)buffer)->createtime;
		statbuf->st_blocks = ((inode*)buffer)->numBlocks;
		log_msg("else stmt of getattr\n");
	}    
	log_msg("leaving getattr\n");
    return retstat;
}

/**
 * Create and open a file
 *
 * If the file does not exist, first create it with the specified
 * mode, and then open it.
 *
 * If this method is not implemented or under Linux kernel
 * versions earlier than 2.6.15, the mknod() and open() methods
 * will be called instead.
 *
 * Introduced in version 2.5
 */
int sfs_create(const char *path, mode_t mode, struct fuse_file_info *fi){
    int retstat = 0;
    fprintf(stderr,"create");
    log_msg("\nsfs_create(path=\"%s\", mode=0%03o, fi=0x%08x)\n",
	    path, mode, fi);
    
    
    return retstat;
}

/** Remove a file */
int sfs_unlink(const char *path){
   fprintf(stderr,"unlink");
    int retstat = 0;
    log_msg("sfs_unlink(path=\"%s\")\n", path);

    
    return retstat;
}

/** File open operation
 *
 * No creation, or truncation flags (O_CREAT, O_EXCL, O_TRUNC)
 * will be passed to open().  Open should check if the operation
 * is permitted for the given flags.  Optionally open may also
 * return an arbitrary filehandle in the fuse_file_info structure,
 * which will be passed to all file operations.
 *
 * Changed in version 2.2
 */
int sfs_open(const char *path, struct fuse_file_info *fi){
	fprintf(stderr,"open");
    int retstat = 0;
    log_msg("\nsfs_open(path\"%s\", fi=0x%08x)\n",
	    path, fi);

    
    return retstat;
}

/** Release an open file
 *
 * Release is called when there are no more references to an open
 * file: all file descriptors are closed and all memory mappings
 * are unmapped.
 *
 * For every open() call there will be exactly one release() call
 * with the same flags and file descriptor.  It is possible to
 * have a file opened more than once, in which case only the last
 * release will mean, that no more reads/writes will happen on the
 * file.  The return value of release is ignored.
 *
 * Changed in version 2.2
 */
int sfs_release(const char *path, struct fuse_file_info *fi){
    fprintf(stderr,"release");
    int retstat = 0;
    log_msg("\nsfs_release(path=\"%s\", fi=0x%08x)\n",
	  path, fi);
    

    return retstat;
}

/** Read data from an open file
 *
 * Read should return exactly the number of bytes requested except
 * on EOF or error, otherwise the rest of the data will be
 * substituted with zeroes.  An exception to this is when the
 * 'direct_io' mount option is specified, in which case the return
 * value of the read system call will reflect the return value of
 * this operation.
 *
 * Changed in version 2.2
 */
int sfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi){
    fprintf(stderr,"read");
    int retstat = 0;
    log_msg("\nsfs_read(path=\"%s\", buf=0x%08x, size=%d, offset=%lld, fi=0x%08x)\n",
	    path, buf, size, offset, fi);

   
    return retstat;
}

/** Write data to an open file
 *
 * Write should return exactly the number of bytes requested
 * except on error.  An exception to this is when the 'direct_io'
 * mount option is specified (see read operation).
 *
 * Changed in version 2.2
 */
int sfs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi){
    fprintf(stderr,"write");
    int retstat = 0;
    log_msg("\nsfs_write(path=\"%s\", buf=0x%08x, size=%d, offset=%lld, fi=0x%08x)\n",
	    path, buf, size, offset, fi);
    
    
    return retstat;
}


/** Create a directory */
int sfs_mkdir(const char *path, mode_t mode){
    fprintf(stderr,"mkdir");
    int retstat = 0;
    log_msg("\nsfs_mkdir(path=\"%s\", mode=0%3o)\n",
	    path, mode);
   
    
    return retstat;
}


/** Remove a directory */
int sfs_rmdir(const char *path){
    fprintf(stderr,"rmdir");
    int retstat = 0;
    log_msg("sfs_rmdir(path=\"%s\")\n",
	    path);
    
    
    return retstat;
}


/** Open directory
 *
 * This method should check if the open operation is permitted for
 * this  directory
 *
 * Introduced in version 2.3
 */
int sfs_opendir(const char *path, struct fuse_file_info *fi){
    int retstat = 0;
    fprintf(stderr, "opendir\n");
    log_msg("\nsfs_opendir\n");//(path=\"%s\", fi=0x%08x)\n",path, fi);
    
    
    return retstat;
}

/** Read directory
 *
 * This supersedes the old getdir() interface.  New applications
 * should use this.
 *
 * The filesystem may choose between two modes of operation:
 *
 * 1) The readdir implementation ignores the offset parameter, and
 * passes zero to the filler function's offset.  The filler
 * function will not return '1' (unless an error happens), so the
 * whole directory is read in a single readdir operation.  This
 * works just like the old getdir() method.
 *
 * 2) The readdir implementation keeps track of the offsets of the
 * directory entries.  It uses the offset parameter and always
 * passes non-zero offset to the filler function.  When the buffer
 * is full (or an error happens) the filler function will return
 * '1'.
 *
 * Introduced in version 2.3
 */
int sfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi){
    log_msg("\nsfs_readdir: %s\n", path);
	int retstat = 0;
	
	//
	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0); 

	log_msg("in readdir\n");	
	
	char buffer [BLOCK_SIZE];
	int block = findInode(path);
	if(block == -1){
		fprintf(stderr,"Couldn't find file\n");
		log_msg("Couldn't find file\n");
		return -1; //TODO: is this right?
	}
	else{
		block_read(block, buffer);
		if(((inode*)buffer)->type == DIR_NODE){
			//read each inode in block and get name
			//filler(buf, "name", &stat, 0);
		}
		else{
			fprintf(stderr, "Not a directory\n");
			log_msg("Not a directory\n");
		}
	}
    
    return retstat;
}

/** Release directory
 *
 * Introduced in version 2.3
 */
int sfs_releasedir(const char *path, struct fuse_file_info *fi){
    int retstat = 0;
	fprintf(stderr,"releasedir");
    
    return retstat;
}

struct fuse_operations sfs_oper = {
  .init = sfs_init,
  .destroy = sfs_destroy,

  .getattr = sfs_getattr,
  .create = sfs_create,
  .unlink = sfs_unlink,
  .open = sfs_open,
  .release = sfs_release,
  .read = sfs_read,
  .write = sfs_write,

  .rmdir = sfs_rmdir,
  .mkdir = sfs_mkdir,

  .opendir = sfs_opendir,
  .readdir = sfs_readdir,
  .releasedir = sfs_releasedir
};

void sfs_usage(){
    fprintf(stderr, "usage:  sfs [FUSE and mount options] diskFile mountPoint\n");
    abort();
}

int main(int argc, char *argv[]){
	fprintf(stderr, "in main\n");
    int fuse_stat;
    struct sfs_state *sfs_data;
    // sanity checking on the command line
    if ((argc < 3) || (argv[argc-2][0] == '-') || (argv[argc-1][0] == '-'))
	sfs_usage();

    sfs_data = malloc(sizeof(struct sfs_state));
    if (sfs_data == NULL) {
		perror("main calloc");
		abort();
    }
    // Pull the diskfile and save it in internal data
    sfs_data->diskfile = argv[argc-2];
    argv[argc-2] = argv[argc-1];
    argv[argc-1] = NULL;
    argc--;
    
    sfs_data->logfile = log_open();
    
    // turn over control to fuse
    fprintf(stderr, "about to call fuse_main, %s \n", sfs_data->diskfile);
    fuse_stat = fuse_main(argc, argv, &sfs_oper, sfs_data);
    fprintf(stderr, "fuse_main returned %d\n", fuse_stat);
    
    return fuse_stat;
}
