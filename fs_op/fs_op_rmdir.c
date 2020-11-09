/*
 * fs_op_rmdir.c
 *
 * description: fs_rmdir function for CS 5600 / 7600 file system
 *
 * CS 5600, Computer Systems, Northeastern CCIS
 * Peter Desnoyers, November 2016
 * Philip Gust, March 2019, March 2020
 */

#include <sys/select.h>

#include "fs_util_dir.h"
#include "fs_util_path.h"
#include "fsx600.h"

/**
 * rmdir - remove a directory.
 *
 * Errors
 *   -ENOENT   - file does not exist
 *   -ENOTDIR  - component of path not a directory
 *   -ENOTDIR  - path not a directory
 *   -ENOTEMPTY - directory not empty
 *
 * @param path the path of the directory
 * @return 0 if successful, or -error number
 */
int fs_rmdir(const char* path)
{
	// get directory inode and leaf
    char leaf[FS_FILENAME_SIZE];
    int dir_inum = get_inode_of_path_dir(path, leaf);

    /* ensure inode exists */
    if (dir_inum < 0) {
        return dir_inum;
    }

    int status = do_rmdir(dir_inum, leaf);
    return status;
}

