/*
 * file:        mktest.c
 * description: create test image for CS 5600 / 7600
 * file system volume.
 *
 * Use FS_VERSION compilation flag to select file
 * system version:
 *   FS_VERSION=0 -- no links or ".", ".." entries
 *   FS_VERSION-1 -- with links and ".", ".." entries
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <sys/select.h>
#include <time.h>
#include <unistd.h>

#include "fsx600.h"

char *disk;
fd_set *inode_map;
fd_set *block_map;
void *next_ptr;

/**
 * Generate test image.
 *
 * @param argc number of arguments including program name
 * @param argv argv[1] is name of image file
 * @return
 */
int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "You must provide an image\n");
        exit(1);
    }

    int i;
    char *file = argv[1];

    int n_blks = 1024;
//    int n_map_blks = 1;
    int n_inos = 64;
    int n_ino_blks = n_inos * sizeof(struct fs_inode) / FS_BLOCK_SIZE;

    disk = malloc(n_blks * FS_BLOCK_SIZE);
    memset(disk, 0, n_blks * FS_BLOCK_SIZE);

    struct fs_super *sb = (void*)disk;
    void *ptr = disk + FS_BLOCK_SIZE;

    inode_map = ptr; ptr += FS_BLOCK_SIZE;
    block_map = ptr; ptr += FS_BLOCK_SIZE;

    // set superblock
    *sb = (struct fs_super){.magic = FS_MAGIC, .inode_map_sz = 1,
            .inode_region_sz = n_ino_blks, .block_map_sz = 1,
            .num_blocks = n_blks, .root_inode = 1};
    FD_SET(0, inode_map);  // inode 0 unused

    /* remember (from /usr/include/i386-linux-gnu/bits/stat.h)
     *    S_IFDIR = 0040000 - directory
     *    S_IFREG = 0100000 - regular file
     */
    /* block 0 - superblock
     *       1 - inode map
     *       2 - block map
     *       3,4,5,6 - inodes
     *       7 - root directory (inode 1)
     *      [8 - file]
     */

    struct fs_inode *inodes = ptr; ptr += 4*FS_BLOCK_SIZE;

    /* root directory
     */
    int inum = 1;
    int root_inum = inum++;
    FD_SET(root_inum, inode_map); // root inode allocated
    int root_blk = (ptr - (void*)disk) / FS_BLOCK_SIZE;
    struct fs_dirent *root_de = ptr; ptr += FS_BLOCK_SIZE;

    int t = 0x50000000;
    inodes[root_inum] = (struct fs_inode){.uid = 1000, .gid = 1000, .mode = 0040777,
            .ctime = t, .mtime = t,
            .size = 0, .nlink = 0,
            .direct = {root_blk, 0, 0, 0, 0, 0},
            .indir_1 = 0, .indir_2 = 0};


#if (FS_VERSION > 0)
    /*  "/.", link to "/" directory
     */
    root_de[0] = (struct fs_dirent){.valid = 1, .isDir = 1,
                                    .inode = root_inum, .name = "."};
    inodes[root_inum].size += sizeof(struct fs_dirent);
    inodes[root_inum].nlink++;	// link for .

    /*  "/..", link to "/" directory (special for root "..")
     */
    root_de[1] = (struct fs_dirent){.valid = 1, .isDir = 1,
                                    .inode = root_inum, .name = ".."};
    inodes[root_inum].size += sizeof(struct fs_dirent);
    inodes[root_inum].nlink++;	// link for ..
#endif  /* FS_VERSION > 0 */

    /*  "/file.A", 1000 bytes, permission 777
     */
    int f1_inode = inum++;
    root_de[2] = (struct fs_dirent){.valid = 0, .isDir = 0,
            .inode = 1717, .name = "file.A"};

    root_de[3] = (struct fs_dirent){.valid = 1, .isDir = 0,
            .inode = f1_inode, .name = "file.A"};
    int f1_blk = (ptr - (void*)disk) / FS_BLOCK_SIZE;
    void *f1_ptr = ptr; ptr += FS_BLOCK_SIZE;

    memset(f1_ptr, 'A', 1000);
    inodes[f1_inode] = (struct fs_inode){.uid = 1000, .gid = 1000, .mode = 0100777,
            .ctime = t+200, .mtime = t+200,
            .size = 1000, .nlink = 1,
            .direct = {f1_blk, 0, 0, 0, 0, 0},
            .indir_1 = 0, .indir_2 = 0};
    // add "file.A" entry size to "/" directory
    inodes[root_inum].size += sizeof(struct fs_dirent);
#if (FS_VERSION > 0)
    inodes[f1_inode].nlink++;

    /*  "/file_link.A", same inode as "file.A"
     */
    root_de[4] = (struct fs_dirent){.valid = 1, .isDir = 0,
                                    .inode = f1_inode, .name = "file_link.A"};
    // add "file_link.A" entry size to "/" directory
    inodes[root_inum].size += sizeof(struct fs_dirent);
    inodes[f1_inode].nlink++;
#endif  /* FS_VERSION > 0 */

    /* "/dir1/", directory, permission 755
     * note invalid directory entry for testing...
     */
    int d1_inode = inum++;
    root_de[5] = (struct fs_dirent){.valid = 0, .isDir = 1,
            .inode = f1_inode, .name = "dir1"};
    root_de[6] = (struct fs_dirent){.valid = 1, .isDir = 1,
            .inode = d1_inode, .name = "dir1"};
    int d1_blk = (ptr - (void*)disk) / FS_BLOCK_SIZE;
    struct fs_dirent *d1_de = ptr; ptr += FS_BLOCK_SIZE;

    inodes[d1_inode] = (struct fs_inode){.uid = 1000, .gid = 1000, .mode = 0040755,
            .ctime = t+400, .mtime = t+400,
            .size = 0, .nlink = 1,
            .direct = {d1_blk, 0, 0, 0, 0, 0},
            .indir_1 = 0, .indir_2 = 0};
    // add "dir1" entry size to "/" directory
    inodes[root_inum].size += sizeof(struct fs_dirent);
#if (FS_VERSION > 0)
    inodes[d1_inode].nlink++;  // link from root directory

    /*  "/dir1/.", link to "/dir1" current directory
     */
    d1_de[0] = (struct fs_dirent){.valid = 1, .isDir = 1,
                                    .inode = d1_inode, .name = "."};
    inodes[d1_inode].size += sizeof(struct fs_dirent);
    inodes[d1_inode].nlink++;	// link to d1 directory

    /*  "/..", link to "/" parent directory
     */
    d1_de[1] = (struct fs_dirent){.valid = 1, .isDir = 1,
                                    .inode = root_inum, .name = ".."};
    inodes[d1_inode].size += sizeof(struct fs_dirent);
    inodes[root_inum].nlink++;  // back link to root directory
#endif  /* FS_VERSION > 0 */

    /* "/dir1/file.2", file, 2012 bytes
     */
    int f2_inode = inum++;
    int f2_blk1 = (ptr - (void*)disk) / FS_BLOCK_SIZE;
    void *f2_ptr = ptr; ptr += FS_BLOCK_SIZE;
    int f2_blk2 = (ptr - (void*)disk) / FS_BLOCK_SIZE; ptr += FS_BLOCK_SIZE;

    d1_de[3] = (struct fs_dirent){.valid = 1, .isDir = 0,
            .inode = f2_inode, .name = "file.2"};

    memset(f2_ptr, '2', 2 * FS_BLOCK_SIZE);
    inodes[f2_inode] = (struct fs_inode){.uid = 1000, .gid = 1000, .mode = 0100777,
            .ctime = t+200, .mtime = t+200,
            .size = 2012, .nlink = 1,
            .direct = {f2_blk2, f2_blk1, 0, 0, 0, 0},
            .indir_1 = 0, .indir_2 = 0};
    // add "file.2" entry size to "/dir1" directory
    inodes[d1_inode].size += sizeof(struct fs_dirent);

#if (FS_VERSION > 1)
    /*  "/dir1/file_sym.2", 6 bytes, permission 777
     */
    int f2sym_inode = inum++;
    d1_de[4] = (struct fs_dirent){.valid = 1, .isDir = 0,
                                  .inode = f2sym_inode, .name = "file_sym.2"};
    int f2sym_blk = (ptr - (void*)disk) / FS_BLOCK_SIZE;
    void *f2sym_ptr = ptr; ptr += FS_BLOCK_SIZE;

    const char* s2_name = d1_de[3].name;
    int s2_name_len = strlen(s2_name);
    memcpy(f2sym_ptr, s2_name, s2_name_len);  // "file.2"
    inodes[f2sym_inode] = (struct fs_inode){.uid = 1000, .gid = 1000, .mode = 0120777, //symlink
                                         .ctime = t+200, .mtime = t+200,
                                         .size = s2_name_len, .nlink = 1,
                                         .direct = {f2sym_blk, 0, 0, 0, 0, 0},
                                         .indir_1 = 0, .indir_2 = 0};
    // add "file.A" entry size to "/" directory
    inodes[d1_inode].size += sizeof(struct fs_dirent);
    inodes[f2sym_inode].nlink++;
#endif  /* FS_VERSION > 0 */

    /* "/dir1/file.0", zero-length file
     */
    int f3_inode = inum++;
    d1_de[5] = (struct fs_dirent){.valid = 1, .isDir = 0,
            .inode = f3_inode, .name = "file.0"};
    inodes[f3_inode] = (struct fs_inode){.uid = 1000, .gid = 1000, .mode = 0100777,
            .ctime = t+200, .mtime = t+200,
            .size = 0, .nlink = 1,
            .direct = {0, 0, 0, 0, 0, 0},
            .indir_1 = 0, .indir_2 = 0};
    // add "file.0" entry size to "/dir1" directory
    inodes[d1_inode].size += sizeof(struct fs_dirent);

    /* "/file.7", 7KB file
     */
    int f4_inode = inum++;
    int f4_indirN = (ptr - (void*)disk) / FS_BLOCK_SIZE;
    int *f4_indir = ptr; ptr += FS_BLOCK_SIZE;
    int f4_blk0 = (ptr - (void*)disk) / FS_BLOCK_SIZE;
    void *f4_data = ptr; ptr += 7*FS_BLOCK_SIZE;

    root_de[7] = (struct fs_dirent){.valid = 1, .isDir = 0,
            .inode = f4_inode, .name = "file.7"};
    inodes[f4_inode] = (struct fs_inode){.uid = 1000, .gid = 1000, .mode = 0100777,
            .ctime = t+300, .mtime = t+300,
            .size = 6*1024 + 500, .nlink = 1,
            .direct = {0, 0, 0, 0, 0, 0},
            .indir_1 = f4_indirN, .indir_2 = 0};
    // fill in the direct block numbers
    for (i = 0; i < 6; i++) {
        inodes[f4_inode].direct[i] = f4_blk0++;
    }
    // add a block number to the indirect block.
    f4_indir[0] = f4_blk0;
    // fill the six full blocks and partial seventh block with '4' characters
    memset(f4_data, '4', 6*1024+500);

    // add file.7 entry size to / directory
    inodes[root_inum].size += sizeof(struct fs_dirent);

    /* "/dir1/file.270", 270KB file
     */
    int f5_inode = inum++;
    int f5_indN1 = (ptr - (void*)disk) / FS_BLOCK_SIZE;
    int *f5_indir1 = ptr; ptr += FS_BLOCK_SIZE;
    int f5_indN2 = (ptr - (void*)disk) / FS_BLOCK_SIZE;
    int *f5_indir2 = ptr; ptr += FS_BLOCK_SIZE;
    int f5_indN2_0 = (ptr - (void*)disk) / FS_BLOCK_SIZE;
    int *f5_indir2_0 = ptr; ptr += FS_BLOCK_SIZE;

    int f5_blk0 = (ptr - (void*)disk) / FS_BLOCK_SIZE;
    void *f5_data = ptr; ptr += 270*FS_BLOCK_SIZE;

    d1_de[6] = (struct fs_dirent){.valid = 1, .isDir = 0,
            .inode = f5_inode, .name = "file.270"};
    inodes[f5_inode] = (struct fs_inode){.uid = 1000, .gid = 1000, .mode = 0100777,
            .ctime = t+300, .mtime = t+300,
            .size = 269*1024 + 721, .nlink = 1,
            .direct = {0, 0, 0, 0, 0, 0},
            .indir_1 = 0, .indir_2 = 0};
    // fill in the direct block numbers
    for (i = 0; i < 6; i++) {
        inodes[f5_inode].direct[i] = f5_blk0++;
    }
    // add the indirect block number
    inodes[f5_inode].indir_1 = f5_indN1;
    // add block numbers to the indirect block
    for (i = 0; i < 256; i++) {
        f5_indir1[i] = f5_blk0++;
    }

    /* 270 - 6 - 256 = 8
     */
    inodes[f5_inode].indir_2 = f5_indN2;
    f5_indir2[0] = f5_indN2_0;
    for (i = 0; i < 8; i++) {
        f5_indir2_0[i] = f5_blk0++;
    }

    memset(f5_data, 'K', 269*1024+721);

    // add "file.270" entry size to "/dir1" directory
    inodes[d1_inode].size += sizeof(struct fs_dirent);


    // mark inodes allocated in inode map
    for (i = 0; i < inum; i++) {
        FD_SET(i, inode_map);
    }
    // mark blocks allocated in the block map
    for (i = 0; i < (ptr - (void*)disk)/FS_BLOCK_SIZE; i++) {
        FD_SET(i, block_map);
    }

    int fd = open(file, O_WRONLY|O_CREAT|O_TRUNC, 0777);
    write(fd, disk, n_blks * FS_BLOCK_SIZE);
    close(fd);

    return 0;
}
