/*
 * fs_util_path.c
 *
 * description: path utility functions for CS 5600 / 7600 file system
 *
 * CS 5600, Computer Systems, Northeastern CCIS
 * Peter Desnoyers, November 2016
 * Philip Gust, March 2019, March 2020
 */

#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "fs_util_dir.h"
#include "fs_util_path.h"
#include "fs_util_vol.h"
#include "split.h"

/** File path delimiter character */
const char* PATH_DELIM = "/";

/**
 * State information for path resolution.
 */
struct path_state {
    /** array of path tokens */
    char** pathtoks;
    /** length of path array */
    int pathlen;
    /** 1 to omit leaf element, 0 otherwise */
    int noleaf;
    /** current path array index */
    int idx;
    /** symlink count */
    int symcount;
    /** current path inum */
    int inum;
};

/**
 * Initialize path_state struct for path.
 * If path ends in path separator, adds leaf
 * path element "." to ensure that leaf is
 * not an empty string.
 *
 * @param ps the path_state
 * @param path the path
 * @param noleaf 1 to omit leaf element 0 otherwise
 */
static void init_path_state(struct path_state* ps, const char* path, int noleaf) {
    ps->inum = fs.root_inode;
    ps->symcount = 0;
    ps->idx = 0;

    // add '.' to end of path if ends with PATH_DELIM
    char lpath[strlen(path)+2];
    strcpy(lpath, path);
    int idx = strlen(path) - strlen(PATH_DELIM);
    if (idx >= 0) {
        if (strstr(path+idx, PATH_DELIM) != NULL) { // ends with path delimiter
            strcat(lpath, ".");  // append "." current directory
        }
    }

    ps->noleaf = (noleaf != 0);  // 1 if noleaf, 0 otherwise
    ps->pathlen = split(lpath, NULL, 0, PATH_DELIM);  // count path names
    ps->pathtoks = malloc(ps->pathlen*sizeof(char*));
    split(lpath, ps->pathtoks, ps->pathlen, PATH_DELIM);
}

/**
 * Release path state struct resources.
 *
 * @param ps the path state
 */
static void release_path_state(struct path_state* ps) {
    free_split_tokens(ps->pathtoks, ps->pathlen);
    memset(ps, 0, sizeof(struct path_state));
}

/**
 * Advances path state.
 *
 * @param ps the path state
 * @param inum the current inode number
 * @return 0 if finished, < 0 if error, > 0 for next inode number
 */
static int next_path_state(struct path_state* ps, int inum) {
    if (ps->idx >= (ps->pathlen - ps->noleaf)) {
        return 0;
    }
    // get inode for named element
    ps->inum = get_dir_entry_inode(inum, ps->pathtoks[ps->idx]);
    return ps->inum;
}

/**
 * Return inode number for specified file or directory path.
 * Given "/a/b/c", returns the inode number for "c".
 *
 * Errors
 *   -ENOENT  - a component of the path is not present.
 *   -ENOTDIR - an intermediate component of path not a directory
 *
 * @param ps the file path state
 * @return inum of path inode or error
 */
static int get_inode_of_path_state(struct path_state* ps)
{
    int inum = ps->inum;  // initial inum
    int status;
    // status == 0 for normal termination, < 0 for error
    while ((status = next_path_state(ps, inum)) > 0) {
        // resolve next symlink path element
        status = 0;
        inum = ps->inum;	// accept inode of current token
        ps->idx++;	// advance to next path token
    }

    // return inum or error status
    return (status < 0) ? status : inum;
}

/**
 * Return inode number for specified file or directory path.
 * Given "/a/b/c", returns the inode number for "c".
 *
 * Errors
 *   -ENOENT  - a component of the path is not present.
 *   -ENOTDIR - an intermediate component of path not a directory
 *
 * @param path the file path
 * @return inode of path node or error
 */
int get_inode_of_path(const char* path)
{
    // initialize path state
    struct path_state ps;
    init_path_state(&ps, path, 0);	// include leaf

    int inum = get_inode_of_path_state(&ps);
    release_path_state(&ps);
    return inum;
}

/**
 *  Return inode number of directory for specified file
 *  or directory path, and a leaf name that may not yet
 *  exist. Given "/a/b/c", returns inode number for "b"
 *  and the leaf "c".
 *
 * Errors
 *   -ENOENT  - a component of the path is not present.
 *   -ENOTDIR - an intermediate component of path not a directory
 *
 * @param path the file path
 * @param leaf pointer to space for FS_FILENAME_SIZE leaf name
 * @return inode of path node or -error
 */
int get_inode_of_path_dir(const char* path, char* leaf)
{
    struct path_state ps;
    init_path_state(&ps, path, 1);  // noleaf

    int inum = get_inode_of_path_state(&ps); // nochild

    // if resolution successful, record path leaf
    if (inum > 0) {
        strcpy(leaf, (ps.pathlen == 0) ? "" : ps.pathtoks[ps.pathlen-1]);
    }

    release_path_state(&ps);
    return inum;
}
