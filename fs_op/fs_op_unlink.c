/*
 * fs_op_unlink.c
 *
 * description: fs_unlink function for CS 5600 / 7600 file system
 *
 * CS 5600, Computer Systems, Northeastern CCIS
 * Peter Desnoyers, November 2016
 * Philip Gust, March 2019, March 2020
 */

#include "fs_util_file.h"
#include "fs_util_path.h"
#include "fs_util_vol.h"

/**
 * unlink - delete a file.
 *
 * Errors
 *   -ENOENT   - file does not exist
 *   -ENOTDIR  - component of path not a directory
 *   -EISDIR   - cannot unlink a directory
 *
 * @param path path to file
 * @return 0 if successful, or -error number
 */
int fs_unlink(const char* path)
{
	// get directory inode and leaf
    char leaf[FS_FILENAME_SIZE];
    int dir_inum = get_inode_of_path_dir(path, leaf);

    /* ensure inode exists */
    if (dir_inum < 0) {
        return dir_inum;
    }

    // unlink entry
    int status = do_unlink(dir_inum, leaf);
    return status;
}
