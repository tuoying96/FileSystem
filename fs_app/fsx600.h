/*
 * file:        fsx600.h
 *
 * description: Data structures for CS 5600/7600 file system.
 *
 * CS 5600, Computer Systems, Northeastern CCIS
 * Peter Desnoyers,  November 2016
 * Philip Gust, March 2019
 */
#ifndef __FSX600_H__
#define __FSX600_H__

#include <stdint.h>

enum {
    /** file system block size in bytes */
	FS_BLOCK_SIZE = 1024,
    /** magic number for superblock */
	FS_MAGIC = 0x37363030
};


enum {
    /** max file name length */
    FS_FILENAME_SIZE = 28
};

/**
 *  Entry in a directory
 */
struct fs_dirent {
    /** entry valid flag */
    uint32_t valid : 1;
    /** entry is directory flag */
    uint32_t isDir : 1;
    /** entry inode */
    uint32_t inode : 30;
    /** file name with trailing NUL */
    char name[FS_FILENAME_SIZE];
};	/* total 32 bytes */

/**
 * Superblock - holds file system parameters.
 */
struct fs_super {
    /** magic number */
    uint32_t magic;
    /** inode map size in blocks */
    uint32_t inode_map_sz;
    /** inode region size in blocks */
    uint32_t inode_region_sz;
    /** block map size in blocks */
    uint32_t block_map_sz;
    /** total blocks, including SB, bitmaps, inodes */
    uint32_t num_blocks;
    /** always inode 1 */
    uint32_t root_inode;

    /* pad out to an entire block */
    char pad[FS_BLOCK_SIZE - 6 * sizeof(uint32_t)]; 
};	/* total FS_BLOCK_SIZE bytes */

enum {
    /** number direct entries */
    N_DIRECT = 6
};

/**
 * Inode - holds file entry information
 */
struct fs_inode {
    /** user ID of file owner */
    uint16_t uid;
    /** group ID of file owner */
    uint16_t gid;
    /** permissions | type: file, directory, ... */
    uint32_t mode;
    /** creation time */
    uint32_t ctime;
    /** last modification time */
    uint32_t mtime;
    /** size in bytes */
    uint32_t size;
    /** number of links */
    uint32_t nlink;
    /** direct block pointers */
    uint32_t direct[N_DIRECT];
    /** single indirect block pointer */
    uint32_t indir_1;
    /** double indirect block pointer */
    uint32_t indir_2;
    /** 64 bytes per inode */
    uint32_t pad[2];

};  /* total 64 bytes */

/**
 * Constants for blocks
 */
enum {
    /** directory entries per block */
    DIRENTS_PER_BLK = FS_BLOCK_SIZE / sizeof(struct fs_dirent),
    /** inodes per block */
	INODES_PER_BLK = FS_BLOCK_SIZE / sizeof(struct fs_inode),
    /** inode pointers per block */
    PTRS_PER_BLK = FS_BLOCK_SIZE / sizeof(uint32_t),
    /** bits per block */
	BITS_PER_BLK = FS_BLOCK_SIZE * 8
};

#endif  /* __FSX600_H__ */


