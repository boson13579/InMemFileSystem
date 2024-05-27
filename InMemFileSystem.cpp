#define FUSE_USE_VERSION 30
#include <bits/stdc++.h>
#include <fcntl.h>
#include <fuse.h>
#include <sys/stat.h>
#include <unistd.h>

#include "Encryption.h"
#include "Filesystem.h"

bool debug = true;

InMemoryFileSystem *filesystem = nullptr;

static int ramfs_getattr(const char *path, struct stat *stbuf) {
    // if(debug) std::cerr << "getattr " << path << std::endl;
    if (std::string(path) == "/") {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        stbuf->st_atime = time(nullptr);
        stbuf->st_mtime = time(nullptr);
        stbuf->st_ctime = time(nullptr);
        stbuf->st_uid = getuid();
        stbuf->st_gid = getgid();
        return 0;
    }

    Directory *parent = filesystem->findParentDirectory(path);
    if (!parent) return -ENOENT;

    std::string name = filesystem->splitPath(path).back();
    if (parent->directories.find(name) != parent->directories.end()) {
        Directory *dir = parent->directories[name];
        stbuf->st_mode = S_IFDIR | dir->mode;
        stbuf->st_nlink = 2;
        return 0;
    }

    if (parent->files.find(name) != parent->files.end()) {
        File *file = parent->files[name];
        stbuf->st_mode = S_IFREG | file->mode;
        stbuf->st_nlink = 1;
        stbuf->st_size = file->size;
        stbuf->st_atime = file->atime;
        stbuf->st_mtime = file->mtime;
        stbuf->st_ctime = file->ctime;
        return 0;
    }

    return -ENOENT;
}

static int ramfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
    (void)offset;
    (void)fi;

    Directory *dir = (std::string(path) == "/") ? filesystem->root : filesystem->findParentDirectory(path)->directories[filesystem->splitPath(path).back()];
    if (!dir) return -ENOENT;

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);
    for (const auto &entry : dir->directories) {
        filler(buf, entry.first.c_str(), NULL, 0);
    }
    for (const auto &entry : dir->files) {
        filler(buf, entry.first.c_str(), NULL, 0);
    }

    return 0;
}

static int ramfs_mkdir(const char *path, mode_t mode) {
    Directory *parent = filesystem->findParentDirectory(path);
    if (!parent) return -ENOENT;

    std::string dir_name = filesystem->splitPath(path).back();
    if (parent->directories.find(dir_name) != parent->directories.end() or
        parent->files.find(dir_name) != parent->files.end()) {
        return -EEXIST;
    }

    parent->directories[dir_name] = new Directory(dir_name, mode);
    return 0;
}

static int ramfs_rmdir(const char *path) {
    Directory *parent = filesystem->findParentDirectory(path);
    if (!parent) return -ENOENT;

    std::string dir_name = filesystem->splitPath(path).back();
    if (parent->directories.find(dir_name) == parent->directories.end()) {
        return -ENOENT;
    }

    delete parent->directories[dir_name];
    parent->directories.erase(dir_name);
    return 0;
}

static int ramfs_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    if (debug) std::cerr << "create " << path << std::endl;
    Directory *parent = filesystem->findParentDirectory(path);
    if (!parent) return -ENOENT;

    std::string file_name = filesystem->splitPath(path).back();
    if (parent->directories.find(file_name) != parent->directories.end() or
        parent->files.find(file_name) != parent->files.end()) {
        return -EEXIST;
    }

    parent->files[file_name] = new File(file_name, mode);
    return 0;
}

static int ramfs_open(const char *path, struct fuse_file_info *fi) {
    if (debug) std::cerr << "open " << path << std::endl;
    Directory *parent = filesystem->findParentDirectory(path);
    if (!parent) return -ENOENT;

    std::string file_name = filesystem->splitPath(path).back();
    if (parent->files.find(file_name) == parent->files.end()) {
        return -ENOENT;
    }
    File *file = parent->files[file_name];
    if(file -> size == 0) return 0;
    if (file -> key != NULL) return -ETXTBSY;

    unsigned char *key = getkey();
    unsigned char plain[4096];
    int plansize = decrypt(file->contents[0].data.data(), file->contents[0].data.size(), key, file->contents[0].iv.data(), plain);
    if(plansize <= 0) return -EACCES;
     
    file -> key = key;
    return 0;
}

static int ramfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    if (debug) std::cerr << "read " << path << std::endl;
    Directory *parent = filesystem->findParentDirectory(path);
    if (!parent) return -ENOENT;

    std::string file_name = filesystem->splitPath(path).back();
    if (parent->files.find(file_name) == parent->files.end()) {
        return -ENOENT;
    }

    File *file = parent->files[file_name];
    if(!file -> size) return 0;

    // AES decrypt
    auto key = file -> key == NULL ? getkey() : file -> key;
    if (key == NULL) return -EAGAIN;
    unsigned char plain[4096];
    int plansize = decrypt(file->contents[offset].data.data(), file->contents[offset].data.size(), key, file->contents[offset].iv.data(), plain);
    if(plansize <= 0) return -EACCES;
    memcpy(buf, plain, size);
    file->atime = time(nullptr);
    return size;
}

static int ramfs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    if (debug) std::cerr << "write " << path << std::endl;
    Directory *parent = filesystem->findParentDirectory(path);
    if (!parent) return -ENOENT;

    std::string file_name = filesystem->splitPath(path).back();
    if (parent->files.find(file_name) == parent->files.end()) {
        return -ENOENT;
    }
    File *file = parent->files[file_name];

    unsigned char predata[4096];
    if (offset % 4096 != 0) {
        off_t oldoff = offset / 4096 * 4096;
        int plansize = decrypt(file->contents[oldoff].data.data(), file->contents[oldoff].data.size(), file->key, file->contents[oldoff].iv.data(), predata);
        if(plansize <= 0) return -EACCES;
    }

    memcpy(predata + offset % 4096, buf, size);
    unsigned char iv[16];
    RAND_bytes(iv, 16);
    unsigned char *key = file -> key == NULL ? getkey() : file -> key;
    if (key == NULL) return -EAGAIN;
    unsigned char chiper[4096];  // if file larger than 4096, it will be cut and add offset
    int chiper_size = encrypt(predata, offset%4096+size, key, iv, chiper);

    file->contents[offset / 4096 * 4096].size = offset + size;
    file->contents[offset / 4096 * 4096].iv = std::vector<unsigned char>(iv, iv + 16);
    file->contents[offset / 4096 * 4096].data = std::vector<unsigned char>(chiper, chiper + chiper_size);

    file->size = 0;
    for (auto [off, content] : file->contents) {
        file->size += content.size;
    }
    file->mtime = time(nullptr);

    return size;
}

static int ramfs_truncate(const char *path, off_t size) {
    if (debug) std::cerr << "truncate " << path << std::endl;
    Directory *parent = filesystem->findParentDirectory(path);
    if (!parent) return -ENOENT;

    std::string file_name = filesystem->splitPath(path).back();
    if (parent->files.find(file_name) == parent->files.end()) {
        return -ENOENT;
    }

    File *file = parent->files[file_name];
    file->size = size;
    file->mtime = time(nullptr);

    return 0;
}

static int ramfs_unlink(const char *path) {
    Directory *parent = filesystem->findParentDirectory(path);
    if (!parent) return -ENOENT;

    std::string file_name = filesystem->splitPath(path).back();
    if (parent->files.find(file_name) == parent->files.end()) {
        return -ENOENT;
    }

    File *file = parent->files[file_name];
    delete file;
    parent->files.erase(file_name);
    return 0;
}

static int ramfs_utimens(const char *path, const struct timespec ts[2]) {
    Directory *parent = filesystem->findParentDirectory(path);
    if (!parent) return -ENOENT;

    std::string file_name = filesystem->splitPath(path).back();
    if (parent->files.find(file_name) == parent->files.end()) {
        return -ENOENT;
    }

    File *file = parent->files[file_name];
    file->atime = ts[0].tv_sec;
    file->mtime = ts[1].tv_sec;
    return 0;
}

static int ramfs_release(const char *path, struct fuse_file_info *fi) {
    if (debug) std::cerr << "release " << path << std::endl;

    Directory *parent = filesystem->findParentDirectory(path);
    if (!parent) return -ENOENT;

    std::string file_name = filesystem->splitPath(path).back();
    if (parent->files.find(file_name) == parent->files.end()) {
        return -ENOENT;
    }

    parent->files[file_name]->key = NULL;

    return 0;
}
static struct fuse_operations ramfs_oper = {
    .getattr = ramfs_getattr,
    .mkdir = ramfs_mkdir,
    .unlink = ramfs_unlink,
    .rmdir = ramfs_rmdir,
    .truncate = ramfs_truncate,
    .open = ramfs_open,
    .read = ramfs_read,
    .write = ramfs_write,
    .release = ramfs_release,
    .readdir = ramfs_readdir,
    .create = ramfs_create,
    .utimens = ramfs_utimens,
};

int main(int argc, char *argv[]) {
    ERR_load_crypto_strings();
    OpenSSL_add_all_algorithms();
    filesystem = new InMemoryFileSystem();
    return fuse_main(argc, argv, &ramfs_oper, NULL);
}
