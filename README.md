# assignment-4
Starter code for assignment 4. 

This repository includes the following source file directories

* **fs_app/**: source files for main file system application (misc.c) and supporting source files
* **fs_op/**: source files that implement Fuse file system operation functions
* **fs_util/**: source files for directory, file, metadata, path, and utility functions
* **img_app/**: source files for suport programs to create and verify file system images

The repository also contains scripts that can be use to faciltate testing within the interactive command shell

* **scripts/**: script files for use by interactive command shell for testing

The repository 'docs' directory contains generated documentation form doxygen.

* **[docs](https://pages.github.ccs.neu.edu/2020FACS5600SV/assignment-4/html)/**: generated documentation from doxygen

For those who are using CLion for development, the repository includes a CMakeLists.txt file with configurations
for building and running the main system application and the image utility programs withn the same project.
Comments within the file show the parameters to use with corresponding CLion configurations.

* **CMakeLists.txt**: CMake file for building and running file system application image file programs

Note that the compile-time definitions in the file include build settings for MacOS. The *_DARWIN_US_64_BIT_INODE* 
definition can be omitted for Windows and Linux. Adjust the include and lib directories and the library name
for the correct platform. You must also ensure the dynamic link loader can find the shared library. On Windows,
that means adding the fuse 'lib' directory to the system path using the Control Panel. See the notes for the
lecture on libraries.

Note: do not use fuse3, as its API has changed and it will not work with this assignment. Use a version of 
fuse 2.7 or later instead.

The *FS_VERSION* value 0 is for the intial version of the file system. Once you have implemented hard links and 
added "." and ".." entries to directories, set *FS_VERSION* to 1 to enable supporting functionality in the code 
and the supporting image utilities.                                                           
