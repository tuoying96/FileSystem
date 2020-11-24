/*
 * fs_util_file.h
 *
 * description: file utility functions for CS 5600 / 7600 file system
 *
 * CS 5600, Computer Systems, Northeastern CCIS
 * Peter Desnoyers, November 2016
 * Philip Gust, March 2019, March 2020
 */

#ifndef FS_UTIL_FILE_H_
#define FS_UTIL_FILE_H_

#include <stdlib.h>
#include <sys/stat.h>

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
int get_file_blkno(int inum, int n, int alloc);

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
int get_file_blk(int inum, int n, void* block, int alloc);

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
int do_read(int inum, char* buf, size_t len, off_t offset);

/**
 *  write bytes of content to an inode.
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
int do_write(int inum, const char* buf, size_t len, off_t offset);


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
int do_truncate(int inum, int len);

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
              int dstdir_inum, const char* dst_leaf);

/**
 * Fill in a stat structure for an inode.
 *
 * @param inum inode number
 * @param sb pointer to stat structure
 */
void do_stat(int inum, struct stat *sb);

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
int init_new_inode(mode_t mode, unsigned ftype);

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
int do_mkentry(int dir_inum, const char* leaf, mode_t mode, unsigned ftype);

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
int do_unlink(int dir_inum, const char* leaf);


int do_link(int src_inum, const char* dir, const char* leaf);

int increment_link_count(int inum);

int decrement_link_count(int inum);

#endif /* FS_UTIL_FILE_H_ */
