/*
 * fs_op_rename.c
 *
 * description: fs_rename function for CS 5600 / 7600 file system
 *
 * CS 5600, Computer Systems, Northeastern CCIS
 * Peter Desnoyers, November 2016
 * Philip Gust, March 2019, March 2020
 */

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#include "fs_util_dir.h"
#include "fs_util_file.h"
#include "fs_util_path.h"
#include "fs_util_vol.h"
#include "blkdev.h"

/**
 * rename - rename a file or directory.
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
int fs_rename(const char* src_path, const char* dst_path)
{
    // get directory inode and leaf for source path
    char src_leaf[FS_FILENAME_SIZE];
    int srcdir_inum = get_inode_of_path_dir(src_path, src_leaf);

	// get directory inode and leaf for dest path
    char dst_leaf[FS_FILENAME_SIZE];
    int dstdir_inum = get_inode_of_path_dir(dst_path, dst_leaf);

    // rename entry
    int status = do_rename(srcdir_inum, src_leaf, dstdir_inum, dst_leaf);
    return status;
}

