#include <iostream>
#include <fuse.h>
#include <unistd.h>
#include <cstring>
#include "../libWad/Wad.h"
#include "../libWad/FileNode.h"

static int my_getattr(const char *path, struct stat *stbuf);
static int my_mknod(const char *path, mode_t mode, dev_t rdev);
static int my_mkdir(const char *path, mode_t mode);
static int my_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
static int my_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
static int my_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi);

static struct fuse_operations operations = {
	.getattr = my_getattr,
	.mknod = my_mknod,
	.mkdir = my_mkdir,
	.read = my_read,
	.write = my_write,
	.readdir = my_readdir,
};

int my_getattr(const char *path, struct stat *stbuf){
    // Retrieve the Wad instance from FUSE context
    Wad* myWad = static_cast<Wad*>(fuse_get_context()->private_data);

    uid_t mounting_user = fuse_get_context()->uid;

    memset(stbuf, 0, sizeof(struct stat));
    // Now you can use myWad as needed...
    // Example: Check if the path is a directory or file in your Wad file system
    if (myWad->isDirectory(path)) {
        // Set the attributes for a directory
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2; // Standard for directories
	stbuf->st_uid = mounting_user; // Set owner UID
        stbuf->st_gid = mounting_user; // Set group GID
    } else if (myWad->isContent(path)){
        // Set the attributes for a file
        stbuf->st_mode = S_IFREG | 0755;
        stbuf->st_nlink = 1;
        stbuf->st_size = myWad->getSize(path); // Assuming getSize returns the file size
        stbuf->st_uid = mounting_user; // Set owner UID
        stbuf->st_gid = mounting_user; // Set group GID
    }
    else return -ENOENT;

    return 0;
}

int my_mknod(const char *path, mode_t mode, dev_t rdev){
    Wad* myWad = static_cast<Wad*>(fuse_get_context()->private_data);
    myWad->createFile(path);
    return 0;
}

int my_mkdir(const char* path, mode_t mode){
    Wad* myWad = static_cast<Wad*>(fuse_get_context()->private_data);
    myWad->createDirectory(path);
    return 0;
}

int my_read(const char* path, char* buf, size_t size, off_t offset, struct fuse_file_info *fi){
    Wad* myWad = static_cast<Wad*>(fuse_get_context()->private_data);
    return myWad->getContents(path, buf, size, offset);
}

static int my_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi){
    Wad* myWad = static_cast<Wad*>(fuse_get_context()->private_data);
    myWad->writeToFile(path, buf, size, offset);
    return size;
}

static int my_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi){
    Wad* myWad = static_cast<Wad*>(fuse_get_context()->private_data);

    if (strcmp(path, "/") != 0) {
        if (!myWad->isDirectory(path)) {
            return -ENOENT;
        }
    }

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);

    std::vector<std::string> contents;
    myWad->getDirectory(path, &contents);
    for (const std::string &entry : contents) {
        filler(buf, entry.c_str(), NULL, 0);
    }

    return 0;
}

int main (int argc, char* argv[]){
	if (argc < 3){
		std::cout << "Not enough arguments." << std::endl;
		exit(EXIT_SUCCESS);
	}

	std::string wadPath = argv[argc-2];


	// relative path!
	if (wadPath.at(0) != '/'){
		wadPath = std::string(get_current_dir_name()) + "/" + wadPath;
	}
	Wad* myWad = Wad::loadWad(wadPath);

	argv[argc - 2] = argv[argc-1];
	argc--;

	// fuse_get_context()->private_data
	return fuse_main(argc, argv, &operations, myWad);
}
