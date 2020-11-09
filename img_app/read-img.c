/*
 * file:        read-img.c
 * description: read and print summary of cs5600/cs7600 file system
 *              volume.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <time.h>

#include "fsx600.h"

/** disk image */
static void *disk;

/** disk superblock */
static struct fs_super *sb;

/** disk block map */
static fd_set *block_map;

/** disk inoode map */
static fd_set *inode_map;

/** tracking block map */
static fd_set *blkmap;

/** tracking inode map */
static fd_set *imap;

/** tracking inode entries list  */
static struct entry { int dir; int inum;} *inode_list;

/** Head and tail of tracking inode entries list */
static int head = 0, tail = 0;

/**
 * Check directory block
 *
 * @param inum directory inode
 * @param blkno directory entry block
 * @return 1 if successful, 0 if error
 */
int check_directory_block(int inum, int blkno) {
    struct fs_dirent *de = disk + blkno * FS_BLOCK_SIZE;
    if (!FD_ISSET(blkno, block_map)) {
        printf("\n***ERROR*** block %d marked free\n", blkno);
    }

    FD_SET(blkno, blkmap);

    // scan directory block
    for (int i = 0; i < DIRENTS_PER_BLK; i++) {
        if (de[i].valid) {
            // report on valid directory entry
            printf("  %s %d %s\n", de[i].isDir ? "D" : "F", de[i].inode,
                   de[i].name);
            int j = de[i].inode;
            if (j < 0 || j >= sb->inode_region_sz * 16) {
                printf("***ERROR*** invalid inode %d\n", j);
                continue;
            }
            // check for loops for directories because
            // other types can have multiple links
            // (should validate link count instead)
            // (also should verify isDir is currect vs. inode)
            if (de[i].isDir) { // only check directory cycles
#if (FS_VERSION > 0)
                // exclude "." and "..", the only ones with multiple links
                if (   (strcmp(de[i].name, ".") == 0)
                       || (strcmp(de[i].name, "..") == 0)) {
                    FD_SET(j, imap);
                    continue;
                }
#endif  /* FS_VERSION */
                if (FD_ISSET(j, imap)) {
                    printf("***ERROR*** loop found (inode %d)\n", inum);
                    return 0;  // failure
                }
            }
            FD_SET(j, imap);
            if (!FD_ISSET(j, inode_map)) {
                printf("***ERROR*** inode %d is marked free\n", j);
            }
            inode_list[head++] = (struct entry) {.dir = de[i].isDir, j};
        }
    }
    printf("\n");
    return 1; // OK
}

#if (FS_VERSION == 0)
/**
 * Check directory and report errors.
 * @param in the inode
 * @param inum the inumber
 * @return 1 if no loop found, 0 if loop found
 */
int check_directory(int inum, struct fs_inode* in) {
    printf("directory: inode %d\n", inum);
    int status = check_directory_block(inum, in->direct[0]);
    return status;
}

#else  /* FS_VERSION > 0 */

/**
 * Check directory and report errors.
 * @param inum the inode number
 * @param in the inode
 * @return 1 if no loop found, 0 if loop found
 */
int check_directory(int inum, struct fs_inode* in) {
    printf("directory: inode %d\n", inum);

    // check direct directory blocks
    for (int i = 0; i < N_DIRECT; i++) {
        if (in->direct[i] == 0) {
            break;
        }
        // found directory block entry
        printf("(block %d)\n", in->direct[i]);
        if (check_directory_block(inum, in->direct[i]) == 0) {
            return 0;
        }
    }

    // check indirect directory blocks
    if (in->indir_1 != 0) {
        // indirect-1 block
        int *indblk_1 = disk + in->indir_1 * FS_BLOCK_SIZE;
        for (int idx1 = 0; idx1 < PTRS_PER_BLK; idx1++) {
            if (indblk_1[idx1] == 0) {  // no more entries in block
                break;
            }
            // found directory block entry
            printf("(block %d)\n", indblk_1[idx1]);
            if (check_directory_block(inum, indblk_1[idx1]) == 0) {
                return 0;
            }
        }
    }

    // check double indirect directory blocks
    if (in->indir_2 != 0) {
        int *indblk_2 = disk + in->indir_2 * FS_BLOCK_SIZE;
        for (int idx2 = 0; idx2 < PTRS_PER_BLK; idx2++) {
            if (indblk_2[idx2] == 0) {
                break;
            }

            // found indirect block
            int *indblk_1 = disk + indblk_2[idx2] * FS_BLOCK_SIZE;
            for (int idx1 = 0; idx1 < PTRS_PER_BLK; idx1++) {
                if (indblk_1[idx1] == 0) {
                    break;
                }
                // found directory block entry
                printf("(block %d)\n", indblk_1[idx1]);
                if (check_directory_block(inum, indblk_1[idx1]) == 0) {
                    return 0;
                }
            }
        }
    }
    return 1;
}
#endif  /* FS_VERSION */

/**
 * Read and print memory summary of cs/5600/7600 file system
 *
 * @param argc number of arguments including program name
 * @param argv argv[1] is name of image file system
 */
int main(int argc, char **argv)
{
    // open image file
    int i, j, fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        perror("can't open");
        exit(1);
    }

    // stat file to get size information
    struct stat _sb;
    if (fstat(fd, &_sb) < 0) {
        perror("fstat");
        exit(1);
    }
    int size = _sb.st_size;

    // read file into memory
    disk = malloc(size);
    if (read(fd, disk, size) != size) {
        perror("read");
        exit(1);
    }
    blkmap = calloc(size/BITS_PER_BLK, 1);
    imap = calloc(size/BITS_PER_BLK, 1);

    // report on superblock
    sb = (void*)disk;
    printf("superblock: magic:  %08x\n"
           "            imap:   %d blocks\n"
           "            bmap:   %d blocks\n"
           "            inodes: %d blocks\n"
           "            blocks: %d\n"
           "            root inode: %d\n\n",
           sb->magic, sb->inode_map_sz, sb->block_map_sz,
           sb->inode_region_sz, sb->num_blocks, sb->root_inode);

    // report on inode map
    printf("allocated inodes: ");
    inode_map = (void*)disk + FS_BLOCK_SIZE;
    char *comma = "";
    for (i = 0; i < sb->inode_map_sz * BITS_PER_BLK; i++) {
        if (FD_ISSET(i, inode_map)) {
            printf("%s %d", comma, i);
            comma = ",";
        }
    }
    printf("\n\n");

    // report on block map
    printf("allocated blocks: ");
    block_map = (void*)inode_map + sb->inode_map_sz * FS_BLOCK_SIZE;
    for (comma = "", i = 0; i < sb->block_map_sz * BITS_PER_BLK; i++) {
        if (FD_ISSET(i, block_map)) {
            printf("%s %d", comma, i);
            comma = ",";
        }
    }
    printf("\n\n");

    // point to inodes
    struct fs_inode *inodes = (void*)block_map + sb->block_map_sz * FS_BLOCK_SIZE;

    int max_inodes = sb->inode_region_sz * INODES_PER_BLK;
    inode_list = calloc(sizeof(struct entry), max_inodes + 100);

    inode_list[head++] = (struct entry){.dir=1, .inum=1};
    FD_SET(1, imap);
    while (head != tail) {
        struct entry e = inode_list[tail++];
        struct fs_inode *in = inodes + e.inum;
        if (!e.dir) {
            // report on inode info
            printf("file: inode %d\n"
                   "      uid/gid %d/%d\n"
                   "      mode %08o\n"
                   "      size  %d\n"
                   "      nlink %d\n",
                   e.inum, in->uid, in->gid, in->mode, in->size, in->nlink);
            printf("blocks: ");

            // report on direct blocks
            for (i = 0; i < N_DIRECT; i++) {
                if (in->direct[i] != 0) {
                    printf("%d ", in->direct[i]);
                    FD_SET(in->direct[i], blkmap);
                    if (!FD_ISSET(in->direct[i], block_map))
                        printf("\n***ERROR*** block %d marked free\n", in->direct[i]);
                }
            }

            // report on single indirect blocks
            if (in->indir_1 != 0) {
                int *buf = disk + in->indir_1 * FS_BLOCK_SIZE;
                for (i = 0; i < PTRS_PER_BLK; i++) {
                    if (buf[i] != 0) {
                        printf("%d ", buf[i]);
                        FD_SET(buf[i], blkmap);
                        if (!FD_ISSET(buf[i], block_map)) {
                            printf("\n***ERROR*** block %d marked free\n", buf[i]);
                        }
                    }
                }
            }

            // report on double indirect blocks
            if (in->indir_2 != 0) {
                int *buf2 = disk + in->indir_2 * FS_BLOCK_SIZE;
                // scan indirect block
                for (i = 0; i < PTRS_PER_BLK; i++) {
                    if (buf2[i] != 0) {
                        // scan double-indirect block
                        int *buf = disk + buf2[i] * FS_BLOCK_SIZE;
                        for (j = 0; j < PTRS_PER_BLK; j++) {
                            if (buf[j] != 0) {
                                printf("%d ", buf[j]);
                                FD_SET(buf[j], blkmap);
                                if (!FD_ISSET(buf[j], block_map)) {
                                    printf("\n***ERROR*** block %d marked free\n", buf[j]);
                                }
                            }
                        }
                    }
                }
            }
            printf("\n\n");
        }
        else {
            // report on directory
            if (!S_ISDIR(in->mode)) {
                printf("***ERROR*** inode %d not a directory\n", e.inum);
                continue;
            }

            if (check_directory(e.inum, in) == 0) goto fail;
        }
    }

    // report on unreachable inodes
    printf("unreachable inodes: ");
    for (i = 1; i < sb->inode_region_sz * 16; i++) {
        if (!FD_ISSET(i, imap) && FD_ISSET(i, inode_map)) {
            printf("%d ", i);
        }
    }
    printf("\n");

    // report on unreachable blocks
    printf("unreachable blocks: ");
    for (i = 1 + sb->inode_map_sz + sb->block_map_sz + sb->inode_region_sz;
         i < sb->num_blocks; i++) {
        if (FD_ISSET(i, blkmap) && !FD_ISSET(i, block_map)) {
            printf("%d ", i);
        }
    }
    printf("\n");

    fail:
    return 0;
}
