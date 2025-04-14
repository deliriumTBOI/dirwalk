#define PATH_MAX 4096
#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <getopt.h>
#include <locale.h>
#include <limits.h> 

typedef struct {
    int showLinks;
    int showDirs;
    int showFiles;
    int needSort;
} Options;

typedef struct {
    char *path;
} FileInfo;

FileInfo *fileList = NULL;
size_t fileCount = 0;
size_t fileListCapacity = 0;
Options options;

int compare(const void *a, const void *b) {
    const FileInfo *fi1 = (const FileInfo *)a;
    const FileInfo *fi2 = (const FileInfo *)b;
    return strcoll(fi1->path, fi2->path);
}

void addFile(const char *fpath) {
    if (fileCount == fileListCapacity) {
        fileListCapacity = (fileListCapacity == 0) ? 16 : fileListCapacity * 2;
        fileList = realloc(fileList, fileListCapacity * sizeof(FileInfo));
        if (fileList == NULL) {
            perror("realloc failed");
            exit(EXIT_FAILURE);
        }
    }

    fileList[fileCount].path = strdup(fpath);
    if (fileList[fileCount].path == NULL) {
        perror("strdup failed");
        exit(EXIT_FAILURE);
    }
    fileCount++;
}

void scanEntry(const char *dirpath, const struct dirent *entry);

void scanDirectory(const char *dirpath) {
    DIR *dir = opendir(dirpath);
    if (dir == NULL) {
        perror("opendir failed");
        return;
    }

    struct dirent *entry;
    errno = 0;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        scanEntry(dirpath, entry); 
    }

    if (errno != 0) {
        perror("readdir failed");
    }
    closedir(dir);
}

void scanEntry(const char *dirpath, const struct dirent *entry) {
    char fullpath[PATH_MAX];

    
    if (strcmp(dirpath, ".") == 0) { 
        snprintf(fullpath, sizeof(fullpath), "%s", entry->d_name);
    } else {
        snprintf(fullpath, sizeof(fullpath), "%s/%s", dirpath, entry->d_name);
    }
    if (strlen(fullpath) >= PATH_MAX) {
        fprintf(stderr, "Path too long: %s\n", fullpath);
        return;
    }

    struct stat st;
    if (lstat(fullpath, &st) == -1) {
        perror("lstat failed");
        return;
    }

    if ((options.showLinks && S_ISLNK(st.st_mode)) ||
        (options.showDirs && S_ISDIR(st.st_mode)) ||
        (options.showFiles && S_ISREG(st.st_mode)) ||
        (!options.showLinks && !options.showDirs && !options.showFiles))
    {
        addFile(fullpath);
    }
    
    if (S_ISDIR(st.st_mode) && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
        scanDirectory(fullpath);
    }
}

Options getOptions(int argc, char *argv[]) {
    Options opts = {0, 0, 0, 0};
    int opt;

    while ((opt = getopt(argc, argv, "ldfs")) != -1) {
        switch (opt) {
            case 'l':
                opts.showLinks = 1;
                break;
            case 'd':
                opts.showDirs = 1;
                break;
            case 'f':
                opts.showFiles = 1;
                break;
            case 's':
                opts.needSort = 1;
                break;
            default:
                fprintf(stderr, "Usage: %s [-ldfs] [dir]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }
    return opts;
}

void sortList() {
    if (options.needSort && fileCount > 0) {
        qsort(fileList, fileCount, sizeof(FileInfo), compare);
    }
}

void printList() {
    for (size_t i = 0; i < fileCount; i++) {
        printf("%s\n", fileList[i].path);
        free(fileList[i].path);
    }
}

int main(int argc, char *argv[]) {
    options = getOptions(argc, argv);
    setlocale(LC_ALL, ""); 

    const char *startDir = (optind < argc) ? argv[optind] : ".";

    struct stat st;
    if (stat(startDir, &st) == -1) {
        perror("stat failed");
        return 1;
    }

    if (S_ISDIR(st.st_mode)) {
        scanDirectory(startDir); 
    } else {
        
        if ((options.showFiles && S_ISREG(st.st_mode)) ||
            (options.showLinks && S_ISLNK(st.st_mode)) ||
            (!options.showLinks && !options.showDirs && !options.showFiles))
        {
            addFile(startDir);
        }
    }

    sortList(); 
    printList(); 

    free(fileList);
    return 0;
}
