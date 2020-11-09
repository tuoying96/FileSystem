/*
 * file:        blkdev.h
 * description: Block device structure for CS 7600 HW3
 *
 * Peter Desnoyers, Northeastern Computer Science, 2015
 * Philip Gust, Northeastern Computer Science, 2019
 */
#ifndef __BLKDEV_H__
#define __BLKDEV_H__

enum {
    /**  block device block size */
    BLOCK_SIZE = 1024
};

/** block device operation status */
enum {
    /** block operation succeeded */
    SUCCESS = 0,
    /** bad block address */
    E_BADADDR = -1,
    /** block unavailable */
    E_UNAVAIL = -2,
    /** bad block size */
    E_SIZE = -3
};

/** Definition of a block device */
struct blkdev {
    /** operations on block device */
    struct blkdev_ops *ops;
    /** block device private state */
    void *private;
};

/** Operations on a block device */
struct blkdev_ops {
    /** number of blocks function */
    int  (*num_blocks)(struct blkdev *dev);
    /** block read function */
    int  (*read)(struct blkdev *dev, int first_blk, int num_blks, void *buf);
    /** block write function */
    int  (*write)(struct blkdev *dev, int first_blk, int num_blks, void *buf);
    /** flush blocks function */
    int  (*flush)(struct blkdev *dev, int first_blk, int num_blks);
    /* close device function */
    void (*close)(struct blkdev *dev);
};

#endif
