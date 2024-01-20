#include <stack>
#include <algorithm>
#include <cstring>
#include "Wad.h"

Wad* Wad::loadWad(const std::string &path) {

    // open the WAD file
    std::ifstream inputFile;
    inputFile.open(path);
    if (!inputFile.is_open()){
        std::cout << "File failed to open." << std::endl;
        return nullptr;
    }

    Wad* wad = new Wad();
    wad->wadFile = path;

    // Read the file header

    // Read magic
    inputFile.read(wad->magic, 4);
    wad->magic[4] = '\0';

    // Read num descriptors
    inputFile.read(reinterpret_cast<char*>(&wad->numDescriptors), sizeof(wad->numDescriptors));

    // Read descriptor length
    inputFile.read(reinterpret_cast<char*>(&wad->descriptorOffset), sizeof(wad->descriptorOffset));

    // jump to the descriptors (jump descriptorOffset bytes forward) and read those
    inputFile.seekg(wad->descriptorOffset);
    for (int i = 0; i < wad->numDescriptors; i++){
        Wad::Descriptor desc{};
        // Read element offset (location of file ASCII's contents)
        inputFile.read(reinterpret_cast<char*>(&desc.elementOffset), sizeof(desc.elementOffset));

        // Read element length
        inputFile.read(reinterpret_cast<char*>(&desc.elementLength), sizeof(desc.elementLength));

        // Read ASCII
        inputFile.read(desc.ascii, sizeof(char) * 8);
        desc.ascii[8] = '\0';

        // debug statement
//        std::cout << "Descriptor " << i << std::endl;
//        std::cout << "Element Offset: " << desc.elementOffset << std::endl;
//        std::cout << "Element Length: " << desc.elementLength << std::endl;
//        std::cout << "Element Ascii: " << desc.ascii << std::endl;
//        std::cout << "---------------------------------------" << std::endl;

        wad->descriptors.push_back(desc);
    }

    inputFile.close();

    // set up tree structure based on descriptors;
    wad->baseDirectory = new FileNode("root", FileNode::Type::NamespaceDirectory, -1, -1, -1);
    wad->baseDirectory->closingDescriptorOffset = wad->descriptorOffset + (16 * wad->numDescriptors);
    int index = -999;
    std::stack<FileNode*> s;
    s.push(wad->baseDirectory);
    for (int i = 0; i < wad->numDescriptors; i++){
        // convert ascii char array to string (ease of use)
        Wad::Descriptor desc = wad->descriptors.at(i);
        std::string givenName = "";
        for (char j : desc.ascii){
            givenName += j;
        }

        // ensure the map marker directory gets 10 files placed inside it
        if (i - 11 == index){
            index = -999;
            s.pop();
        }

        if (givenName.at(0) == 'E' && isdigit(givenName.at(1)) && givenName.at(2) == 'M' && isdigit(givenName.at(3))){ // map marker directory
            auto* newNode = new FileNode(givenName, FileNode::Type::MapDirectory, -1, desc.elementOffset, wad->descriptorOffset + (i * 16));
            s.top()->children.push_back(newNode);
            s.push(newNode);

            index = i;
        }
        else if (givenName.substr(2, 6) == "_START" ){ // namespace directory beginning
            auto* newNode = new FileNode(givenName.substr(0, 2), FileNode::Type::NamespaceDirectory, -1, desc.elementOffset, wad->descriptorOffset + (i * 16));
            s.top()->children.push_back(newNode);
            s.push(newNode);
        }
        else if (givenName.substr(2, 4) == "_END"){ // namespace directory ending
            if (givenName.substr(0, 2) == s.top()->filename.substr(0, 2)){
                s.top()->closingDescriptorOffset = wad->descriptorOffset + (i * 16);
                s.pop();
            }
        }
        else { // generic file
            auto* newNode = new FileNode(givenName, FileNode::Type::StandardFile, desc.elementLength, desc.elementOffset, wad->descriptorOffset + (i * 16));
            s.top()->children.push_back(newNode);
        }
    }
    return wad;
}

std::string Wad::getMagic() {
    return Wad::magic;
}

bool Wad::isContent(const std::string &path) {
    FileNode* thisNode = pathToNode(path, this->baseDirectory);
    if (!thisNode) return false;
    if (thisNode->isStandardFile()) return true;
    return false;
}

FileNode *Wad::pathToNode(std::string path, FileNode* fileNode) {
    if (path == "/") return fileNode;
    if (path == "") return nullptr;
    if (path.at(path.length()-1) == '/') path = path.substr(0, path.length()-1);
    path = path.substr(1, path.length()-1);
    std::string snippedPath = "";
    for (int i = 0; i < path.length(); i++){
        if (path.at(i) == '/') {
            // does fileNode have a child which aligns with snipped Path?
            for (int j = 0; j < fileNode->children.size(); j++){
                std::string str = fileNode->children.at(j)->filename;
                str.erase(std::remove(str.begin(), str.end(), '\0'), str.end());
                if (str == snippedPath){
                    return pathToNode(path.substr(i, path.length()-i), fileNode->children.at(j));
                }
            }
            // if the code reaches this point, the fileNode doesn't have the right child.
            return nullptr;
        }
        snippedPath += path.at(i);
    }
    for (int j = 0; j < fileNode->children.size(); j++){
        std::string str = fileNode->children.at(j)->filename;
        str.erase(std::remove(str.begin(), str.end(), '\0'), str.end());
        if (str == snippedPath){
            return fileNode->children.at(j);
        }
    }
    return nullptr;
}

bool Wad::isDirectory(const std::string &path) {
    FileNode* thisNode = pathToNode(path, this->baseDirectory);
    if (thisNode != nullptr){
        return (thisNode->isMapDirectory() || thisNode->isStandardDirectory());
    }
    return false;
}

int Wad::getSize(const std::string &path) {
    FileNode* thisNode = pathToNode(path, this->baseDirectory);
    if (thisNode != nullptr){
	return thisNode->fileSize;
    }
    return -1;
}

int Wad::getContents(const std::string &path, char *buffer, int length, int offset) {
    FileNode* thisNode = pathToNode(path, this->baseDirectory);
    if (!thisNode) return -1;
    if (!thisNode->isStandardFile()) return -1;

    std::ifstream inputFile;
    inputFile.open(this->wadFile);
    if (!inputFile.is_open()){
        std::cout << "File failed to open." << std::endl;
        return -1;
    }

    // read length bytes from the lump data, starting at offset
    // the given node's (descriptor's) lump data starts at thisNode->fileOffset
    // and the data in question starts in that lump data, at offset
    // so we should jump forward (thisNode->fileOffset + offset) in the wadFile to start reading
    int readPosition = thisNode->fileOffset + offset;
    int actualLength = std::min(length, static_cast<int>(thisNode->fileSize - offset));

    if (actualLength <= 0) {
        inputFile.close();
        return 0; // Offset is beyond the end of the file.
    }

    inputFile.seekg(readPosition);

    // then we want to read length bytes from this point
    inputFile.read(buffer, actualLength);
    int bytesRead = inputFile.gcount();

    inputFile.close();

    return bytesRead;
}

int Wad::getDirectory(const std::string &path, std::vector<std::string> *directory) {
    FileNode* thisNode = pathToNode(path, this->baseDirectory);
    if (!thisNode || thisNode->isStandardFile()) return -1;
    if (thisNode->children.empty()) return 0;
    int count = 0;
    for (int i = 0; i < thisNode->children.size(); i++){
//        std::cout << thisNode->children.at(i)->filename << std::endl;
        std::string str = thisNode->children.at(i)->filename;
        str.erase(std::remove(str.begin(), str.end(), '\0'), str.end());
        // we will probably have to trim the strings of the filenames.
        directory->push_back(str);
        count++;
    }
    return count;
}

void Wad::createDirectory(const std::string &path) {
    // split the path into the existing path and the directory to be created
    int index = -1;
    for (int i = 0; i < path.length(); i++){
        if (path.at(i) == '/' && i != path.length()-1){
            index = i;
        }
    }
    FileNode* thisNode;
    if (index == 0){
	thisNode = pathToNode("/", this->baseDirectory);
    }
    else {
	thisNode = pathToNode(path.substr(0, index), this->baseDirectory);
    }
    if (!thisNode) return; // if the path doesn't exist
    if (!thisNode->isStandardDirectory()) return; // if the path is to a map directory or a file

    std::string newName = path.substr(index + 1, path.length()-index-1); // directory to be created
    if (newName.at(newName.length()-1) == '/') {
	newName = newName.substr(0, newName.length()-1);
    }
    if (newName.size() > 2) return;

    // update the tree to reflect the new directory
    auto* newNode = new FileNode(newName, FileNode::Type::NamespaceDirectory, -1, 0, thisNode->closingDescriptorOffset);
    newNode->closingDescriptorOffset = thisNode->closingDescriptorOffset + 16;
    thisNode->children.push_back(newNode);

    // update the .wad file to reflect the new directory

    // thisNode->endOffset is where the parent Node's closing descriptor is located in the .wad
    long insertPosition = thisNode->closingDescriptorOffset /* compute insert position */;

    int fileSizeInBytes = this->descriptorOffset + (16 * this->numDescriptors);
    int dataShiftSize = fileSizeInBytes-insertPosition; // the data we must shift forward is between our insertPosition and the file's end

    std::vector<char> buffer(dataShiftSize);

    std::fstream wadFile(this->wadFile, std::ios::in | std::ios::out | std::ios::binary);
    if (!wadFile.is_open()) {
        std::cerr << "Failed to open WAD file for updating." << std::endl;
        return;
    }

    // read old data into buffer
    wadFile.seekp(insertPosition);
    wadFile.read(buffer.data(), dataShiftSize);

    // write new data
    wadFile.seekp(insertPosition);
    int length = 0;

    const size_t nameLength = 8;  // Desired length without null terminator

// Function to prepare a fixed-length string
    auto prepareString = [](const std::string& base, const std::string& suffix) {
        std::string result = base + suffix;
        if (result.size() > nameLength) {
            // Truncate if longer than nameLength
            result.resize(nameLength);
        } else if (result.size() < nameLength) {
            // Pad with '\0' if shorter than nameLength
            result.append(nameLength - result.size(), '\0');
        }
        return result;
    };

// Create strings with exactly 8 characters
    std::string startName = prepareString(newName, "_START");
    std::string endName = prepareString(newName, "_END");

    wadFile.write(reinterpret_cast<const char*>(&thisNode->fileOffset), sizeof(thisNode->fileOffset));
    wadFile.write(reinterpret_cast<const char*>(&length), sizeof(length));
    wadFile.write(startName.c_str(), nameLength);

    wadFile.write(reinterpret_cast<const char*>(&thisNode->fileOffset), sizeof(thisNode->fileOffset));
    wadFile.write(reinterpret_cast<const char*>(&length), sizeof(length));
    wadFile.write(endName.c_str(), nameLength);

//     Write back the old data at the new position
    wadFile.seekp(insertPosition + 32);
    wadFile.write(buffer.data(), dataShiftSize);

    // update descriptor count
    wadFile.seekp(4);
    this->numDescriptors += 2;
    wadFile.write(reinterpret_cast<const char *>(&this->numDescriptors), sizeof(this->numDescriptors));

// Close the file
    wadFile.close();

    std::queue<FileNode*> q;
    q.push(this->baseDirectory);
    while (!q.empty()){
	int size = q.size();
	for (int j = 0; j < size; j++){
	    FileNode* current = q.front();
	    q.pop();

	    if (current->descriptorOffset >= insertPosition && current != newNode) current->descriptorOffset += 32;
	    if (current->closingDescriptorOffset >= insertPosition && current != newNode) current->closingDescriptorOffset += 32;
	    for (int i = 0; i < current->children.size(); i++){
		q.push(current->children.at(i));
	    }
	}
    }

//    this = Wad::loadWad(this->wadFile);
}

void Wad::createFile(const std::string &path) {
    // split the path into the existing path and the directory to be created
    int index = -1;
    for (int i = 0; i < path.length(); i++){
        if (path.at(i) == '/' && i != path.length()-1){
            index = i;
        }
    }
    FileNode* thisNode;
    if (index == 0){
	thisNode = pathToNode("/", this->baseDirectory);
    }
    else {
	thisNode = pathToNode(path.substr(0, index), this->baseDirectory); // existing path
    }
    if (!thisNode) return; // if the path doesn't exist
    if (!thisNode->isStandardDirectory()) return; // if the path is to a map directory or a file

    std::string newName = path.substr(index + 1, path.length()-index-1); // directory to be created

    // prevent _start, _end, elmo
    if (newName.length() > 8) return;

    if (newName.at(0) == 'E' && isdigit(newName.at(1)) && newName.at(2) == 'M' && isdigit(newName.at(3))){ // map marker directory
        return;
    }
    if (newName.length() >= 6){
	if (newName.substr(newName.length()-6) == "_START") return;
    }
    if (newName.length() >= 4){
	if (newName.substr(newName.length()-4) == "_END") return;
    }

    // update the tree to reflect the new directory
    auto* newNode = new FileNode(newName, FileNode::Type::StandardFile, 0, 0, thisNode->closingDescriptorOffset);
    thisNode->children.push_back(newNode);

    // thisNode->endOffset is where the parent Node's closing descriptor is located in the .wad
    long insertPosition = thisNode->closingDescriptorOffset /* compute insert position */;

    int fileSizeInBytes = this->descriptorOffset + (16 * this->numDescriptors);
    int dataShiftSize = fileSizeInBytes-insertPosition; // the data we must shift forward is between our insertPosition and the file's end
    std::vector<char> buffer(dataShiftSize);

    std::fstream wadFile(this->wadFile, std::ios::in | std::ios::out | std::ios::binary);
    if (!wadFile.is_open()) {
        std::cerr << "Failed to open WAD file for updating." << std::endl;
        return;
    }

    // read old data into buffer
    wadFile.seekp(insertPosition);
    wadFile.read(buffer.data(), dataShiftSize);

    // write new data
    wadFile.seekp(insertPosition);
    int length = 0;

    const size_t nameLength = 8;  // Desired length without null terminator

    if (newName.size() < nameLength){
        newName.append(nameLength - newName.size(), '\0');
    }

    wadFile.write(reinterpret_cast<const char*>(&length), sizeof(length));
    wadFile.write(reinterpret_cast<const char*>(&length), sizeof(length));
    wadFile.write(newName.c_str(), nameLength);

    // Write back the old data at the new position
    wadFile.seekp(insertPosition + 16);
    wadFile.write(buffer.data(), dataShiftSize);

    // update descriptor count
    wadFile.seekp(4);
    this->numDescriptors += 1;
    wadFile.write(reinterpret_cast<const char *>(&this->numDescriptors), sizeof(this->numDescriptors));

    // Close the file
    wadFile.close();


    std::queue<FileNode*> q;
    q.push(this->baseDirectory);
    while (!q.empty()){
	int size = q.size();
	for (int j = 0; j < size; j++){
	    FileNode* current = q.front();
	    q.pop();

	    if (current->descriptorOffset >= insertPosition && current != newNode) current->descriptorOffset += 16;
	    if (current->closingDescriptorOffset >= insertPosition && current != newNode) current->closingDescriptorOffset += 16;
	    for (int i = 0; i < current->children.size(); i++){
		q.push(current->children.at(i));
	    }
	}
    }
}

int Wad::writeToFile(const std::string &path, const char *buffer, int length, int offset) {
    // split the path into the existing path and the directory to be created
//    int index = -1;
//    for (int i = 0; i < path.length(); i++){
//        if (path.at(i) == '/' && i != path.length()-1){
//            index = i;
//        }
//    }

//    std::cout << path.substr(index, path.length()-index) << std::endl;
    FileNode* thisNode = pathToNode(path, this->baseDirectory);
    if (!thisNode){
	return -1; // if the path doesn't exist
    }
    if (!thisNode->isStandardFile()) {
	return -1; // if the path is not to a file
    }
    if (thisNode->fileSize != 0) return 0; // non-empty file

//    std::string newName = path.substr(index + 1, path.length()-index-1); // directory to be created
//    std::cout << thisNode->filename << " " << newName << std::endl;

    // Step 2: Open the file
    std::fstream wadFile(this->wadFile, std::ios::in | std::ios::out | std::ios::binary);
    if (!wadFile.is_open()) {
	std::cout << "File failed to open" << std::endl;
        return -1;
    }

    int newOffset = this->descriptorOffset - offset;
    int fileSizeInBytes = this->descriptorOffset + (16 * this->numDescriptors);
    int dataShiftSize = fileSizeInBytes-newOffset; // the data we must shift forward is between our insertPosition and the file's end

    std::vector<char> tempBuffer(dataShiftSize);
    wadFile.seekg(newOffset);
    wadFile.read(tempBuffer.data(), dataShiftSize);

    wadFile.seekg(newOffset);
    std::streampos beforeWrite = wadFile.tellp();
    wadFile.write(buffer, length);
    std::streampos afterWrite = wadFile.tellp();

    wadFile.write(tempBuffer.data(), dataShiftSize);

    int bytesWritten = static_cast<int>(afterWrite-beforeWrite);

    this->descriptorOffset += bytesWritten;

    thisNode->fileSize = bytesWritten;
    thisNode->fileOffset = newOffset;

    std::queue<FileNode*> q;
    q.push(this->baseDirectory);
    while (!q.empty()){
	int size = q.size();
	for (int j = 0; j < size; j++){
	    FileNode* current = q.front();
	    q.pop();

	    if (current->descriptorOffset >= newOffset) current->descriptorOffset += bytesWritten;
	    if (current->closingDescriptorOffset >= newOffset) current->closingDescriptorOffset += bytesWritten;
	    if (current->fileOffset >= newOffset && current != thisNode) current->fileOffset += bytesWritten;
	    for (int i = 0; i < current->children.size(); i++){
		q.push(current->children.at(i));
	    }
	}
    }

    wadFile.seekg(8);
    wadFile.write(reinterpret_cast<const char *>(&this->descriptorOffset), sizeof(this->descriptorOffset));
    wadFile.seekg(thisNode->descriptorOffset);
    wadFile.write(reinterpret_cast<const char *>(&newOffset), sizeof(newOffset));
    wadFile.seekg(thisNode->descriptorOffset + 4);
    wadFile.write(reinterpret_cast<const char *>(&bytesWritten), sizeof(bytesWritten));

    return bytesWritten;
}

