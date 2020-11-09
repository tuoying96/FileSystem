/*
 * fs_op_getattr.c
 *
 * description: fs_getattr function for CS 5600 / 7600 file system
 *
 * CS 5600, Computer Systems, Northeastern CCIS
 * Peter Desnoyers, November 2016
 * Philip Gust, March 2019, March 2020
 */

#include "fs_util_dir.h"
#include "fs_util_file.h"
#include "fs_util_path.h"

/**
 * getattr - get file or directory attributes. For a description of
 * the fields in 'struct stat', see 'man lstat'.
 *
 * Note - fields not provided in CS5600fs are:
 *    st_nlink - always set to 1
 *    st_atime, st_ctime - set to same value as st_mtime
 *
 * Errors
 *   -ENOENT  - a component of the path is not present.
 *   -ENOTDIR - an intermediate component of path not a directory
 *
 * @param path the file path
 * @param sb pointer to stat struct
 * @return 0 if successful, or -error number
 */
int fs_getattr(const char* path, struct stat *sb)
{
	// get inode for specified path
    char leaf[PATH_MAX];
    int inum = get_inode_of_path_dir(path, leaf);

    // error if not found
    if (inum < 0) {
        return inum;
    }

    // report error if cannot get inode of file
    inum = get_dir_entry_inode(inum, leaf);
    if (inum < 0) {
        return inum;
    }

    // fill stat struct if value
    do_stat(inum, sb);

    return 0;
}

