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
    int show_links;
    int show_dirs;
    int show_files;
    int sort_output;
} Options;

typedef struct {
    char *path;
} FileInfo;

FileInfo *file_list = NULL;
size_t file_count = 0;
size_t file_list_capacity = 0;
Options options;

int fileinfo_compare(const void *a, const void *b) {
    const FileInfo *fi1 = (const FileInfo *)a;
    const FileInfo *fi2 = (const FileInfo *)b;
    return strcoll(fi1->path, fi2->path);
}

void add_file_info(const char *fpath) {
    if (file_count == file_list_capacity) {
        file_list_capacity = (file_list_capacity == 0) ? 16 : file_list_capacity * 2;
        file_list = realloc(file_list, file_list_capacity * sizeof(FileInfo));
        if (file_list == NULL) {
            perror("realloc failed");
            exit(EXIT_FAILURE);
        }
    }

    file_list[file_count].path = strdup(fpath);
    if (file_list[file_count].path == NULL) {
        perror("strdup failed");
        exit(EXIT_FAILURE);
    }
    file_count++;
}

void process_entry(const char *dirpath, const struct dirent *entry);

void process_directory(const char *dirpath) {
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
        process_entry(dirpath, entry); 
    }

    if (errno != 0) {
        perror("readdir failed");
    }
    closedir(dir);
}

void process_entry(const char *dirpath, const struct dirent *entry) {
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

    if ((options.show_links && S_ISLNK(st.st_mode)) ||
        (options.show_dirs && S_ISDIR(st.st_mode)) ||
        (options.show_files && S_ISREG(st.st_mode)) ||
        (!options.show_links && !options.show_dirs && !options.show_files))
    {
        add_file_info(fullpath);
    }
    
    if (S_ISDIR(st.st_mode) && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
        process_directory(fullpath);
    }
}

Options parse_options(int argc, char *argv[]) {
    Options opts = {0, 0, 0, 0};
    int opt;

    while ((opt = getopt(argc, argv, "ldfs")) != -1) {
        switch (opt) {
            case 'l':
                opts.show_links = 1;
                break;
            case 'd':
                opts.show_dirs = 1;
                break;
            case 'f':
                opts.show_files = 1;
                break;
            case 's':
                opts.sort_output = 1;
                break;
            default:
                fprintf(stderr, "Usage: %s [-ldfs] [dir]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }
    return opts;
}

void sort_file_list() {
    if (options.sort_output && file_count > 0) {
        qsort(file_list, file_count, sizeof(FileInfo), fileinfo_compare);
    }
}

void print_file_list() {
    for (size_t i = 0; i < file_count; i++) {
        printf("%s\n", file_list[i].path);
        free(file_list[i].path);
    }
}

int main(int argc, char *argv[]) {
    options = parse_options(argc, argv);
    setlocale(LC_ALL, ""); 

    const char *start_dir = (optind < argc) ? argv[optind] : ".";

    struct stat st;
    if (stat(start_dir, &st) == -1) {
        perror("stat failed");
        return 1;
    }

    if (S_ISDIR(st.st_mode)) {
        process_directory(start_dir); 
    } else {
        
        if ((options.show_files && S_ISREG(st.st_mode)) ||
            (options.show_links && S_ISLNK(st.st_mode)) ||
            (!options.show_links && !options.show_dirs && !options.show_files))
        {
            add_file_info(start_dir);
        }
    }

    sort_file_list(); 
    print_file_list(); 

    free(file_list);
    return 0;
}
