#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <string>
#include <unordered_map>
#include <vector>

struct Content {
    int size; //plantext size
    std::vector<unsigned char> data; // chiper
    std::vector<unsigned char> iv;

    Content() : size(0) {}
};

class File {
public:
    int size;
    std::string name;
    std::unordered_map<off_t, Content> contents;
    mode_t mode;
    time_t atime;
    time_t mtime;
    time_t ctime;

    File(const std::string &name, mode_t mode);
};

class Directory {
public:
    std::string name;
    mode_t mode;
    time_t atime;
    time_t mtime;
    time_t ctime;
    std::unordered_map<std::string, Directory *> directories;
    std::unordered_map<std::string, File *> files;

    Directory(const std::string &name, mode_t mode);
    ~Directory();
};

class InMemoryFileSystem {
public:
    Directory *root;

    InMemoryFileSystem();
    ~InMemoryFileSystem();

    std::vector<std::string> splitPath(const std::string &path);
    Directory* findParentDirectory(const std::string &path);

private:
    Directory* findDirectory(const std::string &path);
};

#endif // FILESYSTEM_H
