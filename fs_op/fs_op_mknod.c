/*
 * fs_op_mknod.c
 *
 * description: fs_mknod function for CS 5600 / 7600 file system
 *
 * CS 5600, Computer Systems, Northeastern CCIS
 * Peter Desnoyers, November 2016
 * Philip Gust, March 2019, March 2020
 */

#include <stdlib.h>
#include <sys/stat.h>

#include "fs_util_file.h"
#include "fs_util_path.h"
#include "fsx600.h"

/**
 * mknod - create a new file with permissions (mode & 01777)
 * minor device numbers extracted from mode. Behavior undefined
 * when mode bits other than the low 9 bits are used.
 *
 * The access permissions of path are constrained by the
 * umask(2) of the parent process.
 *
 * Errors
 *   -ENOTDIR  - component of path not a directory
 *   -EEXIST   - file already exists
 *   -ENOSPC   - free inode not available
 *   -ENOSPC   - directory full
 *
 * @param path the file path
 * @param mode the mode, indicating block or character-special file
 * @param dev the character or block I/O device specification
 * @return 0 if successful, or -error number
 */
int fs_mknod(const char* path, mode_t mode, dev_t dev)
{
    // get inode of new directory's parent
    char leaf[FS_FILENAME_SIZE];
    int dir_inum = get_inode_of_path_dir(path, leaf);

    // error getting directory inode
    if (dir_inum < 0) {
        return dir_inum;
    }
	// make directory entry
    int inum = do_mkentry(dir_inum, leaf, mode, S_IFREG);
    return (inum < 0) ? inum : 0;
}


