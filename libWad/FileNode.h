//
// Created by Aidan on 11/30/2023.
//

#ifndef LABORATORY_FILENODE_H
#define LABORATORY_FILENODE_H


#include <vector>
#include <string>
#include <queue>
#include <iostream>

struct FileNode {
    enum struct Type {
        StandardFile,
        NamespaceDirectory,
        MapDirectory
    };
    FileNode(std::string filename, Type type, uint32_t fileSize, uint32_t fileOffset, uint32_t descriptorOffset){
        this->filename = filename;
        this->fileType = type;
        this->fileSize = fileSize;
        this->fileOffset = fileOffset;
	this->descriptorOffset = descriptorOffset;
	this->closingDescriptorOffset = -1;
    }
    FileNode() {
        this->filename = "";
    };
    void printBFS(){
        if (!this) {
            return; // Handle the case of an empty tree
        }

        std::queue<FileNode*> q;
        q.push(this); // Start from the root node

        while (!q.empty()) {
            int size = q.size();

            for (int j = 0; j < size; j++) {
                FileNode *current = q.front();
                q.pop();


                // Process the current node
                std::cout << current->filename;

                for (int i = 0; i < current->children.size(); i++) {
                    q.push(current->children.at(i));
                }
            }
            std::cout << std::endl;
        }
        std::cout << "---------------------------" << std::endl;
    }

    bool isMapDirectory() const { return fileType == Type::MapDirectory; }
    bool isStandardDirectory() const { return fileType == Type::NamespaceDirectory; }
    bool isStandardFile() const { return fileType == Type::StandardFile; }

    Type fileType;
    std::vector<FileNode*> children;

    uint32_t fileSize;
    uint32_t fileOffset;
    uint32_t descriptorOffset;
    uint32_t closingDescriptorOffset;
    std::string filename;
};


#endif //LABORATORY_FILENODE_H
