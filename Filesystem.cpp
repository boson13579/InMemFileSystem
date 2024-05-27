#include "Filesystem.h"
#include <ctime>

File::File(const std::string &name, mode_t mode)
    : size(0), name(name), mode(mode), key(nullptr){
    time(&atime);
    time(&mtime);
    time(&ctime);
}

Directory::Directory(const std::string &name, mode_t mode)
    : name(name), mode(mode) {
    time(&atime);
    time(&mtime);
    time(&ctime);
}

Directory::~Directory() {
    for (auto &entry : directories) {
        delete entry.second;
    }
    for (auto &entry : files) {
        delete entry.second;
    }
}

InMemoryFileSystem::InMemoryFileSystem() {
    root = new Directory("/", 0755);
}

InMemoryFileSystem::~InMemoryFileSystem() {
    delete root;
}

std::vector<std::string> InMemoryFileSystem::splitPath(const std::string &path) {
    std::vector<std::string> tokens;
    std::string token;
    size_t start = 0, end = 0;

    while ((end = path.find('/', start)) != std::string::npos) {
        if (end != start) {
            tokens.push_back(path.substr(start, end - start));
        }
        start = end + 1;
    }

    if (start < path.length()) {
        tokens.push_back(path.substr(start));
    }

    return tokens;
}

Directory* InMemoryFileSystem::findParentDirectory(const std::string &path) {
    std::vector<std::string> tokens = splitPath(path);
    if (tokens.empty()) return nullptr;

    std::string parentPath;
    for (size_t i = 0; i < tokens.size() - 1; ++i) {
        parentPath += "/" + tokens[i];
    }

    return findDirectory(parentPath.empty() ? "/" : parentPath);
}

Directory* InMemoryFileSystem::findDirectory(const std::string &path) {
    Directory *dir = root;
    std::vector<std::string> tokens = splitPath(path);

    for (const auto &token : tokens) {
        if (dir->directories.find(token) != dir->directories.end()) {
            dir = dir->directories[token];
        } else {
            return nullptr;
        }
    }

    return dir;
}
