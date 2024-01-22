# WadFS: A FUSE-Based WAD File System Explorer

## Overview
WadFS is a file management project that integrates a custom file system for WAD files using Linux's FUSE (Filesystem in Userspace) software. It allows users to mount WAD files and navigate their contents as they would with a regular file system. The WAD contents include map directories, namespace directories, and ordinary files. The FUSE daemon interfaces with a C++ library which manages the conversion to and from WAD files (1-dimensional array structure) into a flexible directory (multi-level tree structure). It facilitates both reading from and writing to directories and files, along with creation of new files and directories.

## Features
- Mount WAD files as a file system using FUSE
- Perform standard file operations (read, write, create directories/nodes)
- Navigate and manage WAD files as regular directories and files

![image](https://github.com/aidantambling/Fuse-Wad-Explorer/assets/101668617/756e9647-7634-4224-b008-147ce92e17c1)

## Installation
The repository contains makefiles which simplify the process of linking the library and interfacing the daemon. 

First, clone the repository to your machine

```console
git clone https://github.com/aidantambling/Fuse-Wad-Explorer.git
cd Fuse-Wad-Explorer
```

You must also make sure the FUSE library is installed and configured in your Linux system

```console
sudo apt install libfuse-dev fuse
sudo chmod 666 /dev/fuse 
```

You will want to extract the library and daemon directories from the zipped file using this command

```console
tar zxvf wad.tar.gz
```

Next, you will run the 'make' command in the respective directories to generate executables

```console
cd libWad
make
cd ..
cd wadfs
make
cd ..
```

## Usage

With the executables generated, the WAD file can be mounted:

```console
./wadfs/wadfs -s somewadfile.wad /some/mount/directory 
```

Now, /some/mount/directory will be 'created' as a new directory in your system, with its contents reflecting the contents of somewadfile.wad. The WAD file contents can be explored, and new files can be added to the mounted directory / WAD file. This can be accomplished using standard Linux commands in the terminal.

To unmount the WAD file, you can use:

```console
fusermount -u /some/mount/directory
```

Where /some/mount/directory is the name of the directory that was initially mounted.

## Contact
For any queries regarding this project, please contact:

- Aidan Tambling - atambling@ufl.edu
- Project Link: https://github.com/aidantambling/Fuse-Wad-Explorer
