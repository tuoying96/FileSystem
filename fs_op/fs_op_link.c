#include <stdlib.h>
#include <sys/stat.h>

#include "fs_util_file.h"
#include "fs_util_path.h"
#include "fs_util_vol.h"
#include "fsx600.h"
#include <errno.h>
#include <string.h>
#include <fuse.h>

/**
 * created a hard link for an existing file.
 
 * Errors:
 *   -ENOENT   - source file or directory does not exist
 *   -ENOTDIR  - component of source or target path not a directory
 *   -EEXIST   - destination already exists
 *
 * @param src_path the source path
 * @param dst_path the destination path 
 * @return 0 if successful, or -error number
 */

int fs_link(const char* src_path, const char* dst_path)
{
    // get inode for specified path
    int src_inum = get_inode_of_path(src_path);
     // return error code if error
    if (src_inum < 0) 
    {
        return src_inum;
     }
      /* cannot hard link  if it is directory */
    if (S_ISDIR(fs.inodes[src_inum].mode)) {
        return -EISDIR;
    }

    /* get dst last layer directory inode and leaf*/
    char leaf[FS_FILENAME_SIZE];
    int dir_inum = get_inode_of_path_dir(dst_path, leaf);

    // ensure that inode exists
    if (dir_inum < 0)
    {
        return dir_inum;
    }

    /* error if it is not a directory */
    if (!S_ISDIR(fs.inodes[dir_inum].mode)) 
    {
        return -ENOTDIR;
    }

     /* error if the dst file is empty */
    if  (strlen(leaf) == 0)
    {
        return -EACCES;
    }

    // get inode for specified path
    int dst_inum = get_inode_of_path(dst_path);
    //if hard link has exist
    if (dst_inum > 0) 
    {
        return -EEXIST;
    }

    char* dst_dir = (char*)malloc(strlen(dst_path) + 1);
    int copy_size = strlen(dst_path) -  strlen(leaf);
    memcpy(dst_dir, dst_path, copy_size);
    dst_dir[copy_size] = 0;

    do_link(src_inum, dst_dir, leaf);
    
    free(dst_dir);

    return 0;
}
