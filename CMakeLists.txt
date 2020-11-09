cmake_minimum_required(VERSION 3.17)
project(assignment_4 C)

set(CMAKE_C_STANDARD 11)

# global lib directory
link_directories(/usr/local/lib)

# global include directories
include_directories(usr/local/include)
include_directories(/usr/local/include/osxfuse)

# include directories for project source
include_directories(fs_op)
include_directories(fs_util)
include_directories(fs_app)

# definitions required by FUSE
add_compile_definitions(_FILE_OFFSET_BITS=64)
add_compile_definitions(FUSE_USE_VERSION=27)
add_compile_definitions(_DARWIN_USE_64_BIT_INODE)
#definitions required by FS implementation
#  FS_VERSION=0 -- version with no links or ".", ".." entries
#  FS_VERSION-1 -- version with links and ".", ".." entries
add_compile_definitions(FS_VERSION=0)

# compile these source directories
aux_source_directory(fs_op fs_op_src)
aux_source_directory(fs_util fs_util_src)
aux_source_directory(fs_app fs_app_src)

# run command line interactive shell
# working directory: $ProjectFileDir$
# cmd args: -cmdline  -image images/test_image_mktest.img (example)
add_executable(assignment_4 ${fs_op_src} ${fs_util_src} ${fs_app_src} )
target_link_libraries(assignment_4 osxfuse)

# make an empty file system
# working directory: $ProjectFileDir$
# cmd args: -size 10M images/test_image_mkfs.img (example)
add_executable(assignment_4_mkfs img_app/mkfs-x6.c)

# make a test file system
# working directory: $ProjectFileDir$
# cmd args: images/test_image_mktest.img (example)
add_executable(assignment_4_mktest img_app/mktest.c)

# read file system image
# working directory: $ProjectFileDir$
# cmd args: images/test_image_mktest.img (example)
add_executable(assignment_4_read-img img_app/read-img.c)
