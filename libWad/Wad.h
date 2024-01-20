//
// Created by Aidan on 11/30/2023.
//

#ifndef LABORATORY_WAD_H
#define LABORATORY_WAD_H
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include "FileNode.h"

struct Wad {
    //    The Wad class is used to represent WAD data and should have the following functions. The root of all paths
    //    in the WAD data should be "/", and each directory should be separated by '/' (e.g., "/F/F1/LOLWUT").
    struct Descriptor {
        uint32_t elementOffset;
        uint32_t elementLength;
        char ascii[9]; // 8 bits + 1 bit for null terminator
    };

    char magic[5]; // 4 bits + 1 bit for null terminator
    unsigned int numDescriptors;
    unsigned int descriptorOffset;
    std::string wadFile;
    std::vector<Wad::Descriptor> descriptors;
    FileNode* baseDirectory;

    static Wad* loadWad(const std::string &path);
    //    Object allocator; dynamically creates a Wad object and loads the WAD file data from path into memory.
    //    Caller must deallocate the memory using the delete keyword.

    void printDescriptors(){
        for (int i = 0; i < this->numDescriptors; i++){
            std::cout << "Descriptor " << i+1 << ": " << std::endl;
            std::cout << "Element Offset: " << this->descriptors.at(i).elementOffset << std::endl;
            std::cout << "Element Length: " << this->descriptors.at(i).elementLength << std::endl;
            std::cout << "Element ASCII: " << this->descriptors.at(i).ascii << std::endl;
            std::cout << "--------------------------------------" << std::endl;
        }
    }

    std::string getMagic();
    // Returns the magic for this WAD data.

    FileNode* pathToNode(std::string path, FileNode* fileNode);

    bool isContent(const std::string &path);
    //    Returns true if path represents content (data), and false otherwise.
    bool isDirectory(const std::string &path);
    //    Returns true if path represents a directory, and false otherwise.
    int getSize(const std::string &path);
    //    If path represents content, returns the number of bytes in its data; otherwise, returns -1.
    int getContents(const std::string &path, char *buffer, int length, int offset = 0);
    //    If path represents content, copies as many bytes as are available, up to length, of content's data into the preexisting buffer. If offset is provided, data should be copied starting from that byte in the content. Returns
    //    number of bytes copied into buffer, or -1 if path does not represent content (e.g., if it represents a directory).
    int getDirectory(const std::string &path, std::vector<std::string> *directory);
    //    If path represents a directory, places entries for immediately contained elements in directory. The elements
    //    should be placed in the directory in the same order as they are found in the WAD file. Returns the number of
    //    elements in the directory, or -1 if path does not represent a directory (e.g., if it represents content).

    void createDirectory(const std::string &path);
    //    path includes the name of the new directory to be created. If given a valid path, creates a new directory
    //    using namespace markers at path. The two new namespace markers will be added just before the “_END”
    //marker of its parent directory. New directories cannot be created inside map markers.
    void createFile(const std::string &path);
    //path includes the name of the new file to be created. If given a valid path, creates an empty file at path,
    //        with an offset and length of 0. The file will be added to the descriptor list just before the “_END” marker
    //        of its parent directory. New files cannot be created inside map markers.
    int writeToFile(const std::string &path, const char *buffer, int length, int offset = 0);
    //If given a valid path to an empty file, augments file size and generates a lump offset, then writes length amount
    //of bytes from the buffer into the file’s lump data. If offset is provided, data should be written starting from that
    //byte in the lump content. Returns number of bytes copied from buffer, or -1 if path does not represent content
    //        (e.g., if it represents a directory).
};


#endif //LABORATORY_WAD_H
