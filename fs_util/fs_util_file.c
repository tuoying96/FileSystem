/*
 * fs_util_file.c
 *
 * description: file utility functions for CS 5600 / 7600 file system
 *
 * CS 5600, Computer Systems, Northeastern CCIS
 * Peter Desnoyers, November 2016
 * Philip Gust, March 2019, March 2020
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <fuse.h>

#include "fs_util_dir.h"
#include "fs_util_file.h"
#include "fs_util_meta.h"
#include "fs_util_path.h"
#include "fs_util_vol.h"
#include "blkdev.h"
#include "max.h"
#include "min.h"


/** file block of 0s */
static char zeros[FS_BLOCK_SIZE];

/**
 * Returns the block number of the n-th block of the file,
 * or allocates it if it does not exist and alloc == 1. If
 * file was extended, the new block is initialized with 0s.
 *
 * @param inum the number of file inode
 * @param n the 0-based block index in file
 * @param alloc 1=allocate block if does not exist 0 = fail
 *   if does not exist
 * @return block number of the n-th block or 0 if unavailable
 */
int get_file_blkno(int inum, int n, int alloc)
{
    uint32_t buf[PTRS_PER_BLK];

    // get entry from direct blocks
    struct fs_inode *in = &fs.inodes[inum];
    if (n < N_DIRECT) {
        if (in->direct[n] == 0) {
        	if (alloc == 0) {
        		return 0;  // not found if no alloc
        	}
        	// alloc and add block to inode
            int blkno = get_free_blk();
            if (blkno == 0) {  // no space
            	return 0;
            }
            in->direct[n] = blkno;
            mark_inode(inum);
            disk->ops->write(disk, in->direct[n], 1, zeros);
        }
        return in->direct[n];
    }

    // get entry from single-indirect block
    n -= N_DIRECT;
    if (n < PTRS_PER_BLK) {
        if (in->indir_1 == 0) {
        	if (alloc == 0) { // not found if no alloc
        		return 0;
        	}
        	// add single-indirect block
            int blkno = get_free_blk();
            if (blkno == 0) {  // no space
            	return 0;
            }
            in->indir_1 = blkno;
            mark_inode(inum);
            disk->ops->write(disk, in->indir_1, 1, zeros);
        }
        disk->ops->read(disk, in->indir_1, 1, buf);
        if (buf[n] == 0) {
        	if (alloc == 0) {
        		return 0;
        	}
        	// extend single-indirect block
            int blkno = get_free_blk();
            if (blkno == 0) {  // no space
            	return 0;
            }
            buf[n] = blkno;
            disk->ops->write(disk, in->indir_1, 1, buf);
        }
        return buf[n];
    }

    // get entry for indirect blocks
    n -= PTRS_PER_BLK;
    if (n > PTRS_PER_BLK * PTRS_PER_BLK) {
        return 0;
    }
    int m = n / PTRS_PER_BLK;
    int k = n - m * PTRS_PER_BLK;
    if (in->indir_2 == 0) {
    	if (alloc == 0) {  // not found of no alloc
    		return 0;
    	}
        int blkno = get_free_blk();
        if (blkno == 0) {  // no space
        	return 0;
        }
        in->indir_2 = blkno;
        mark_inode(inum);
        disk->ops->write(disk, in->indir_2, 1, zeros);
    }

    // get double-indirect block
    disk->ops->read(disk, in->indir_2, 1, buf);
    if (buf[m] == 0) {
    	if (alloc == 0) {
    		return 0;
    	}
    	// add double-indirect block with new free block
        int blkno = get_free_blk();
        if (blkno == 0) {  // no space
        	return 0;
        }
        buf[m] = blkno;
        disk->ops->write(disk, in->indir_2, 1, buf);
        disk->ops->write(disk, buf[m], 1, zeros);
    }

    // get single-indirect block from double-indirect
    int buf_m = buf[m];
    disk->ops->read(disk, buf_m, 1, buf);
    if (buf[k] == 0) {
    	if (alloc == 0) {  // not found if no alloc
    		return 0;
    	}
    	// add single-indirect block with new free block
        int blkno = get_free_blk();
        if (blkno == 0) {  // no space
        	return 0;
        }
        buf[k] = blkno;
        disk->ops->write(disk, buf_m, 1, buf);
        disk->ops->write(disk, buf[k], 1, zeros);
    }
    return buf[k];
}

/**
 * Gets the n-th block of the file, or allocates it if it
 * does not exist and alloc == 1. If file was extended, new
 * block is initialized with 0s. If block is NULL, equivalent
 * to get_file_blkno().
 *
 * Errors
 *   -EIO  - error reading block
 *
 * @param inum the number of file inode
 * @param n the 0-based block index in file
 * @param block storage for the block read
 * @param alloc 1=allocate block if does not exist 0 = fail
 *   if does not exist
 * @return block number of the n-th block, 0 if unavailable,
 *   or -error number
 */
int get_file_blk(int inum, int n, void* block, int alloc) {
	int blkno = get_file_blkno(inum, n, alloc);

	// read block if found and block storage provided
	if ((blkno > 0) && (block != NULL)) {
		if (disk->ops->read(disk, blkno, 1, block) < 0) {
			// report error if cannot read block
			memset(block, 0, FS_BLOCK_SIZE);
			return -EIO;
		}
    }

	return blkno;
}

/**
 * Read bytes from content of an inode.
 *
 * Should return exactly the number of bytes requested, except:
 *   - if offset >= file len, return 0
 *   - if offset+len > file len, return bytes from offset to EOF
 *   - on error, return <0
 *
 * Errors:
 *   -EIO     - error reading block
 *
 * @param inum the inumber of inode to truncate
 * @param buf the read buffer
 * @param len the number of bytes to read
 * @param offset to start reading at
 * @return number of bytes actually read if successful, or -error number
 */
int do_read(int inum, char* buf, size_t len, off_t offset) {
    // get pointer to inode for inum
    struct fs_inode *in = &fs.inodes[inum];

    // done if offset greater than file size
    if (offset >= in->size) {
        return 0;
    }

    // adjust length to length of file from offset
    if (in->size < offset + len) {
        len = in->size - offset;
    }

    // index of first block
    int blkidx1 = offset / FS_BLOCK_SIZE;
    int blkidx2 = (offset + len) / FS_BLOCK_SIZE;

    // read blocks into buf
    offset -= blkidx1 * FS_BLOCK_SIZE;
    int _len = len;
    for (int blkindex = blkidx1; blkindex <= blkidx2; blkindex++) {
    	// get block for block index
        char blk[FS_BLOCK_SIZE];
        int blkno = get_file_blk(inum, blkindex, blk, 0);

        // report error if not found
        if (blkno <= 0) {
            return -EIO;
        }

        // copy block content to buf
        int l = min(FS_BLOCK_SIZE - offset, len);
        memcpy(buf, &blk[offset], l);

        buf += l;
        len -= l;
        offset = 0;
    }

    return _len;

}

/**
 *  Write bytes of content to an inode.
 *
 * It should return exactly the number of bytes requested, except on
 * error.
 *
 * Errors:
 *   -ENOSPC  - no space in file sysem
 *   -EINVAL  - if 'offset' is greater than current file length.
 *  			(POSIX semantics support the creation of files with
 *  			"holes" in them, but we don't)
 *
 * @param inum the inumber of inode to truncate
 * @param buf the buffer to write
 * @param len the number of bytes to write
 * @param offset the offset to starting writing at
 * @return number of bytes actually written if successful, or -error number
 */
int do_write(int inum, const char* buf, size_t len, off_t offset) {
    // get pointer to inode for inum
    struct fs_inode *in = &fs.inodes[inum];

    // return error code of offset out of range
    if (offset > in->size) {
        return -EINVAL;
    }

    int blkidx1 = offset / FS_BLOCK_SIZE;
    int blkidx2 = (offset + len) / FS_BLOCK_SIZE;

    // write buffer to file blocks
    offset -= blkidx1 * FS_BLOCK_SIZE;
    int _len = len;
    for (int blkindex = blkidx1; blkindex <= blkidx2; blkindex++) {
    	// get block
        char blk[FS_BLOCK_SIZE];
    	int blkno = get_file_blk(inum, blkindex, blk, 1);

    	// return error code if error or out of space
    	if (blkno <= 0) {
            mark_inode(inum);
            flush_metadata();
            return (blkno == 0) ? -ENOSPC : blkno;
        }

        // copy more buffer fuse_parser_data to block
        int l = min(FS_BLOCK_SIZE - offset, len);
        memcpy(&blk[offset], buf, l);

        // write block back to disk
        disk->ops->write(disk, blkno, 1, blk);
        buf += l;
        len -= l;
        offset = 0;
        in->size += l;
        in->mtime = time(NULL);  // OK thorough 2100
    }

    mark_inode(inum);
    flush_metadata();

    return _len - len;
}

/**
 * Truncate fuse_parser_data specified by inode.
 * Currently only length 0 is supported.
 *
 * Errors
 *   -EINVAL  - invalid length argument
 *
 * @param inum the inumber of inode to truncate
 * @param len new length of file -- currently only 0 allowed
 * @return 0 if successful, or -error number
 */
int do_truncate(int inum, int len)
{
    if (len != 0) {
    	return -EINVAL;		/* invalid argument */
    }

    /// get inode for inum
    struct fs_inode *in = &fs.inodes[inum];

	uint32_t buf[PTRS_PER_BLK], buf1[PTRS_PER_BLK];
    int i, j;

    /* unlink double indirect nodes */
    if (in->indir_2) {
        disk->ops->read(disk, in->indir_2, 1, buf);
        for (i = 0; i < PTRS_PER_BLK; i++) {
            if (buf[i] != 0) {  // block allocated
                disk->ops->read(disk, buf[i], 1, buf1);
                for (j = 0; j < PTRS_PER_BLK; j++) {
                    if (buf1[j] != 0) { // block allocated
                        return_blk(buf1[j]);  // free block
                    }
                }
                return_blk(buf[i]);  // return head block
            }
        }
        return_blk(in->indir_2); // free head block
        in->indir_2 = 0;
    }

    /* unlink single indirect nodes */
    if (in->indir_1) {
        disk->ops->read(disk, in->indir_1, 1, buf);
        for (i = 0; i < PTRS_PER_BLK; i++) {
            if (buf[i] != 0) {
                return_blk(buf[i]);  // free head block
            }
        }
        return_blk(in->indir_1);
        in->indir_1 = 0;
    }

    /* unlink direct nodes */
    for (i = 0; i < N_DIRECT; i++) {
        if (in->direct[i] != 0) {
            return_blk(in->direct[i]);  // free direct blocks
            in->direct[i] = 0;
        }
    }

    // reset inode size and modification time
    in->size = 0;
    in->mtime = time(NULL);  // OK thorough 2100

    return 0;
}

/**
 * Rename a file or directory.
 *
 * Note that this is a simplified version of the UNIX rename
 * functionality - see 'man 2 rename' for full semantics. In
 * particular, the full version can move across directories, replace a
 * destination file, and replace an empty directory with a full one.
 *
 * Errors:
 *   -ENOENT   - source file or directory does not exist
 *   -ENOTDIR  - component of source or target path not a directory
 *   -EEXIST   - destination already exists
 *   -EINVAL   - source and destination not in the same directory
 *
 * @param src_path the source path
 * @param dst_path the destination path.
 * @return 0 if successful, or -error number
 */
int do_rename(int srcdir_inum, const char* src_leaf,
              int dstdir_inum, const char* dst_leaf)
{
    /** ensure same source and dest directory */
    if (srcdir_inum == 0 || (srcdir_inum != dstdir_inum)) {
        return -EINVAL;
    }

    /* get source/target directory inode */
    struct fs_inode *din = &fs.inodes[srcdir_inum];

    /* find source directory entry */
    int s_blkno;
    struct fs_dirent s_de[DIRENTS_PER_BLK];
    int s_dirno = get_dir_entry_block(srcdir_inum, s_de, &s_blkno, src_leaf);
    if (s_dirno < 0) {
        return -ENOENT;  // source does not exist
    }

    /* ensure target directory entry does not exist */
    int t_blkno;
    struct fs_dirent t_de[DIRENTS_PER_BLK];
    int t_dirno = get_dir_entry_block(dstdir_inum, t_de, &t_blkno, dst_leaf);
    if (t_dirno >= 0) {
        return -EEXIST;  // destination entry already exists
    }

    // update directory entry
    // truncates leaf at FS_FILENAME_SIZE-1, then '\0'
    strncpy(s_de[s_dirno].name, dst_leaf, FS_FILENAME_SIZE-1);
    disk->ops->write(disk, s_blkno, 1, s_de);
    din->mtime = time(NULL);  // reset modification time
    // OK thorough 2100

    // mark directory inode block dirty and flush to disk
    mark_inode(srcdir_inum);
    flush_metadata();

    return 0;
}

/**
 * Fill in a stat structure for an inode.
 *
 * @param inum inode number
 * @param sb pointer to stat structure
 */
void do_stat(int inum, struct stat *sb)
{
    memset(sb, 0, sizeof(*sb));
    // point to inode for inum
	struct fs_inode *in = &fs.inodes[inum];
	sb->st_ino = inum;
    sb->st_mode = in->mode;
    sb->st_nlink = in->nlink;
    sb->st_uid = in->uid;
    sb->st_gid = in->gid;
    sb->st_size = in->size;
    // number of 512-byte blocks rounded up to nearest block
    sb->st_blocks =  (in->size + 512 - 1) / 512;
    sb->st_atime = sb->st_mtime = in->mtime;
    sb->st_ctime = in->ctime;
}

/**
 * Allocate and initialize new inode with mode and file type.
 *
 *  * Errors
 *   -ENOSPC   - free inode not available
 *
 * @param mode the mode, indicating block or character-special file
 * @param ftype the type of file (S_IFDIR for dir, S_IFREG for regular)
 * @return inum of entry if successful, -error if error
 */
int init_new_inode(mode_t mode, unsigned ftype) {
    // allocate and fill inode for new entry
    int inum = get_free_inode();
    if (inum == 0) {
        return -ENOSPC;	// no free inode
    }
    struct fs_inode *in = &fs.inodes[inum];

    // set S_IFMT field with specified ftype value
    in->mode = ((mode & ~S_IFMT) | (ftype & S_IFMT));
    in->ctime = in->mtime = time(NULL);  // OK thorough 2100
    in->size = 0;
    in->nlink = 0;
    struct fuse_context *ctx = fuse_get_context();
    in->uid = (ctx->pid == 0) ? getuid() : ctx->uid;
    in->gid = (ctx->pid == 0) ? getgid() : ctx->gid;
    mark_inode(inum);

    return inum;
}

/**
 * Make a file system file or directory entry for the file type.
 *
 * Note: use GUD and UID from fuse_context returned
 * by fuse get_fuse_context() function
 *
 * Errors
 *   -ENOTDIR  - dir inode is not a directory
 *   -EEXIST   - directory already exists
 *   -ENOSPC   - free inode not available
 *   -ENOSPC   - results in >32 entries in directory
 *
 * @param dir_inum the inum of the directory inode
 * @param leaf the leaf name in the inode directory
 * @param mode the mode, indicating block or character-special file
 * @param ftype the type of file (S_IFDIR for dir, S_IFREG for regular)
 * @return inum of entry if successful, -error if error
 */
int do_mkentry(int dir_inum, const char* leaf, mode_t mode, unsigned ftype)
{
    /* get pointer to directory inode */
    struct fs_inode *din = &fs.inodes[dir_inum];
    if (!S_ISDIR(din->mode)) {
        return -ENOTDIR;	// path component not directory
    }

    // make sure entry does not exist in directory
    int blkno;
    char buf[FS_BLOCK_SIZE];
	if (get_dir_entry_block(dir_inum, buf, &blkno, leaf) >= 0) {
		return -EEXIST;	// leaf already exists
	}

	// find free directory entry
	int entno = get_dir_free_entry_block(dir_inum, buf, &blkno);
	if (entno < 0) {
		return -ENOSPC;	// no free directory entry
    }

	// init new inode for entry
	int inum = init_new_inode(mode, ftype);
	if (inum < 0) {
		return inum;
	}

	// set directory entry information
	struct fs_dirent *de = (void*)buf;
	set_dir_entry(&de[entno], inum, leaf);

    // write updated directory block to disk
    disk->ops->write(disk, blkno, 1, buf);

    // increment size of directory by one fs_dirent
    din->size += sizeof(struct fs_dirent);
    mark_inode(dir_inum);

    // flush metadata updates
    flush_metadata();

    return inum;
}

/**
 * Remove an file named leaf in the specified
 * directory inum.
 *
 * Errors
 *   -ENOENT   - file does not exist
 *   -ENOTDIR  - component of path not a directory
 *   -EISDIR    - path is a directory
 *   -ENOTEMPTY - directory not empty
 *
 * @param dir_inum the inumber of a directory inode
 * @param leaf the name of a child directory entry
 * @return 0 if successful, or -error number
 */
int do_unlink(int dir_inum, const char* leaf) {
    // ensure inode is a directory
    struct fs_inode *din = &fs.inodes[dir_inum];
    if (!S_ISDIR(din->mode)) {
        return -ENOTDIR;
    }

    /* find directory entry block and index */
    int blkno;
    char buf[FS_BLOCK_SIZE];
    int entno = get_dir_entry_block(dir_inum, buf, &blkno, leaf);
    if (entno < 0) {
        return -ENOENT;
    }

    // get inode of directory entry
    struct fs_dirent *de = (void*)buf;
    int inum = de[entno].inode;

    /* ensure that entry being removed is not a directory */
    if (S_ISDIR(fs.inodes[inum].mode)) {
        return -EISDIR;
    }

    // mark directory entry free and write directory block
    de[entno].valid = 0;
    disk->ops->write(disk, blkno, 1, buf);

    // truncate file to 0 length and mark its inode block dirty
    do_truncate(inum, 0);
    mark_inode(inum);

    // free unlinked file inode
    return_inode(inum);

    // decrease size of directory by one fs_dirent
    // NOTE: add logging to report errors like this
    din->size = max(0, din->size-sizeof(struct fs_dirent));
    mark_inode(dir_inum);

    // flush dirty metadata blocks
    flush_metadata();

    return 0;
}


int do_link(int src_inum, const char* dir, const char* leaf)
{
    char* new_path = (char*)malloc(strlen(dir) + strlen(leaf) + 1);
    sprintf(new_path, "%s%s", dir, leaf);

    int dir_inum = get_inode_of_path_dir(new_path, leaf);
    // error getting directory inode
    if (dir_inum < 0) {
        return dir_inum;
    }

    // make sure entry does not exist in directory
    int blkno;
    char buf[FS_BLOCK_SIZE];
	if (get_dir_entry_block(dir_inum, buf, &blkno, leaf) >= 0) {
		return -EEXIST;	// leaf already exists
	}

	// find free directory entry
	int entno = get_dir_free_entry_block(dir_inum, buf, &blkno);
	if (entno < 0) {
		return -ENOSPC;	// no free directory entry
    }

	// set directory entry information
     increment_link_count(src_inum);
	struct fs_dirent *de = (void*)buf;
	set_dir_entry(&de[entno], src_inum, leaf);

    free(new_path);
    return 0;
}

int increment_link_count(int inum)
{
     /* get pointer to file inode */
    struct fs_inode *din = &fs.inodes[inum];
    if (S_ISDIR(din->mode)) 
    {
        return -EISDIR;	
    }
    din->nlink++;
}

int decrement_link_count(int inum)
{
     /* get pointer to file inode */
    struct fs_inode *din = &fs.inodes[inum];
    if (S_ISDIR(din->mode)) 
    {
        return -EISDIR;	
    }

    if (din->nlink > 0)
    {
        din->nlink--;
    }

    if (din->nlink == 0)
    {
        // free unlinked file inode
        return_inode(inum);
    }
}
