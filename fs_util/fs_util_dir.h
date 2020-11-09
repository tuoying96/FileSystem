/*
 * fs_util_dir.h
 *
 * description: directory utility functions for CS 5600 / 7600 file system
 *
 * CS 5600, Computer Systems, Northeastern CCIS
 * Peter Desnoyers, November 2016
 * Philip Gust, March 2019, March 2020
 */

#ifndef FS_UTIL_DIR_H_
#define FS_UTIL_DIR_H_

#include <errno.h>

#include "fs_util_meta.h"
#include "fsx600.h"

/*
 * Note on path translation errors:
 * In addition to method-specific errors listed below,
 * almost every method can return one of the following
 * errors if it fails to locate a file or directory
 * corresponding to a specified path.
 *
 * Errors
 *   -ENOENT  - a component of the path is not present.
 *   -ENOTDIR - a component of path not a directory
 */

/**
 * Determines whether directory is empty.
 * A directory is empty if it only it
 * contains no entries.
 *
 *  Errors
 *   -ENOTDIR = not a directory
 *
 * @param inum the inumber of the directory inode
 * @return 1 (true) if empty, 0 (false) if has entries
 *   or - error
 */
 int is_dir_empty(int inum);

/**
 * Gets the number of of directory entries.
 *
 *  Errors
 *   -ENOTDIR = not a directory
 *
 * @param inum the inumber of the directory inode
 * @return size if is a directory or -error
 */
int get_dir_entry_count(int inum);

/**
 * Look up a single directory entry in a directory block.
 *
 * Errors
 *   -ENOENT  - a component of the path is not present.
 *
 * @param de storage for a directory block
 * @param name the name of the entry
 * @return entry in directory block or -error
 */
int get_dir_entry_in_block(struct fs_dirent* de, const char* name);

/**
 * Look up a free directory entry in a directory block.
 *
 * Errors
 *   -ENNOSPC  - free entry not found
 *
 * @param de storage for a directory block
 * @return entry in directory block, or -error
 */
int get_free_entry_in_block(struct fs_dirent* de);

/**
 * Look up a single directory entry in a directory.
 *
 * Errors
 *   -EIO     - error reading block
 *   -ENOENT  - a component of the path is not present.
 *   -ENOTDIR - intermediate component of path not a directory
 *
 * @param inum the inode number of a directory
 * @param block storage for a directory block
 * @param blkno block no for a directory block
 * @param name the name of the entry
 * @return entry in block returned through block, or -error
 */
int get_dir_entry_block(
	int inum, void* block, int* blkno, const char* name);

/**
 * Find block with free entry in directory.
 *
 * Errors
 *   -EIO     - error reading block
 *   -ENOENT  - a component of the path is not present.
 *   -ENOTDIR - intermediate component of path not a directory
 *   -ENOSPC  - cannot allocate space for free entry
 *
 * @param inum the inode number of a directory
 * @param block storage for a directory block
 * @param blkno block no returned for a directory block
 * @return entry in block returned through block, or -error
 */
int get_dir_free_entry_block(int inum, void* block, int* blkno);

/**
 * Look up a single directory entry in a directory.
 *
 * Errors
 *   -EIO     - error reading block
 *   -ENOENT  - a component of the path is not present.
 *   -ENOTDIR - intermediate component of path not a directory
 *
 * @param inum the inode number of a directory
 * @param name the name of the entry
 * @return inode number of the entry or -error
 */
int get_dir_entry_inode(int inum, const char* name);

/**
 * Sets directory entry with inum and name
 *
 * @param de the directory entry
 * @param inum the inode number
 * @param name the entry name
 */
void set_dir_entry(struct fs_dirent* de, int inum, const char* name);

/**
 * Remove an empty directory named leaf in the specified
 * directory inum.
 *
 * Errors
 *   -ENOENT   - file does not exist
 *   -ENOTDIR  - component of path not a directory
 *   -ENOTDIR  - path not a directory
 *   -ENOTEMPTY - directory not empty
 *
 * @param dir_inum the inumber of a directory inode
 * @param leaf the name of a child directory entry
 * @return 0 if successful, or -error number
 */
int do_rmdir(int dir_inum, const char* leaf);

#endif /* FS_UTIL_DIR_H_ */
