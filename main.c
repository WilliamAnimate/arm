/*
 * this code could be better in thousands of different ways, but
 * i am too lazy to make it right. by continuing, you agree to the
 * possibility of pain, seizure, a trip to the ER or even death;
 * the latter of which is most likely
 *
 * good luck, and i'll see you on the other side.
 *
 * coperight (c) 1984 name <email@exmaple.com>
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <getopt.h>
#include <string.h> // we're gonna get hacked so hard
#include <sys/stat.h>
#include <dirent.h>

#include "rm.h"

#define PROGRAM_NAME "rm"

struct RmOptions {
    bool dry_run;
    bool recursive;
    bool force;
    bool abort_on_error;
    bool preserve_system_root;
    bool preserve_important_directories;
    bool preserve_user_home;
};

enum FolderType {
    CONTAINS_FILES,
    EMPTY,
    DOES_NOT_EXIST,
    NOT_A_DIR, // i miss Err(T) in rust
    ERROR, // because we dont have good error handling in C, and you can't return -1 here
};

void fAbortIfError(bool should_abort) {
    if (should_abort) {
        puts("aborting on error");
        exit(EXIT_FAILURE);
    }
}

/**
 * you exit manually; im not doing that for you :3
 */
void fPrintError(char* app_name) {
    fprintf(stderr, "Usage: %s [-hrfd] -- file_name\n", app_name);
}

enum FolderType fDirContainsFiles(const char* path) {
    struct stat sPathStat;
    DIR *dDir;
    struct dirent *diEntry;
    int has_files = 0;

    // check if path exists...
    if (stat(path, &sPathStat) != 0) {
        perror("error getting file status");
        return DOES_NOT_EXIST;
    }

    // ... then if its a directory...
    if (!S_ISDIR(sPathStat.st_mode)) {
        return NOT_A_DIR;
    }

    // ... then try to open it...
    dDir = opendir(path);
    if (dDir == NULL) {
        perror("Error opening directory");
        return ERROR;
    }

    // ... to see if it has any files within
    while ((diEntry = readdir(dDir)) != NULL) {
        // skip `.` and `..` entries
        if (strcmp(diEntry->d_name, ".") != 0 && strcmp(diEntry->d_name, "..") != 0) {
            has_files = true;
            break;
        }
    }

    closedir(dDir); // don't memleak now, silly

    if (has_files) {
        return CONTAINS_FILES;
    } else {
        return EMPTY;
    }
}

int fRecursiveDelete(const char* file, struct RmOptions rm) {
    DEBUGPRINT("??? %s\n", file);
    DIR *dDir;
    struct dirent *entry;
    char file_path[PATH_MAX];

    dDir = opendir(file);
    if (dDir == NULL) {
        perror("Error opening directory");
        return 1;
    }

    while ((entry = readdir(dDir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        snprintf(file_path, sizeof(file_path), "%s/%s", file, entry->d_name);

        struct stat path_stat;
        stat(file_path, &path_stat);

        if (S_ISDIR(path_stat.st_mode)) {
            DEBUGPRINT("%s is dir\n", file_path);
            // tail call optimization won't save me now
            fRecursiveDelete(file_path, rm);
            if (rm.dry_run) {
                printf("Would delete %s\n", file_path);
                return 0;
            }
            if (remove(file_path) != 0) {
                perror("Error deleting file");
                return 1;
            }
        }

        if (S_ISREG(path_stat.st_mode)) {
            DEBUGPRINT("%s is regular file\n", file_path);
            if (rm.dry_run) {
                // this doesn't print. FIXME!!
                printf("Would delete %s\n", file_path);
                return 0;
            }
            DEBUGPRINT("trying to remove() %s\n", file_path);
            if (remove(file_path) != 0) {
                perror("Error deleting file");
                return 1;
            }
        }
    }

    closedir(dDir);
    return 0;
}

void fTryDeleteFile(const char* file, struct RmOptions rm) {
    enum FolderType folder_type = fDirContainsFiles(file);
    switch (folder_type) {
        case CONTAINS_FILES:
            if (!rm.recursive) {
                printf("%s: cannot remove '%s': is a directory\n", PROGRAM_NAME, file);
                puts("help: try using -r or --recursive to recursively delete everything within");
                break; // funny story: i forgot a break; here. im a genuine dumbass.
            }
            DEBUGPRINT("deleting recursively...\n");

            // scary!
            // remove files recursively then remove the folder itself.
            if (fRecursiveDelete(file, rm) != 0) {
                perror("Error deleting file");
                fAbortIfError(rm.abort_on_error);
            }
            if (rm.dry_run) {
                printf("Would delete %s", file);
                break;
            }
            if (remove(file) != 0) {
                perror("Error deleting file");
                fAbortIfError(rm.abort_on_error);
            }

            break;
        case EMPTY:
        case NOT_A_DIR:
            if (rm.dry_run) {
                printf("Would delete %s", file);
                break;
            }
            if (remove(file) != 0) {
                perror("Error deleting file");
                fAbortIfError(rm.abort_on_error);
            }
            break;
        case DOES_NOT_EXIST:
            // it's not a bug, but a feature.
            // *((char*)0) = 0;
            // __builtin_unreachable();
            fAbortIfError(rm.abort_on_error);
            break;
        default:
            fprintf(stderr, "Not Implemented");
            fAbortIfError(rm.abort_on_error);
            break;
    }
}

int main(int argc, char *argv[]) {
    // i fukcing forgot what this variable is supposed to represent
    int c;
    int iArgsIndex = 0;

    // whether '--' keyword is already parsed; if so, then don't skip it again and treat it as a file
    // incase user has a file literally called '--'
    bool bAlreadyParsedDoubleDash = false;

    struct RmOptions meow;

    static struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {"recursive", no_argument, 0, 'r'},
        {"force", no_argument, 0, 'f'},
        {"dry", no_argument, 0, 'd'},
        {"dry-run", no_argument, 0, 'd'},
        {0, 0, 0, 0}
    };

    while ((c = getopt_long(argc, argv, "hrfd", long_options, NULL)) != -1) {
        iArgsIndex++;
        switch (c) {
            case 'h':
                printf("Help option\n");
                break;
            case 'r':
                meow.recursive = true;
                printf("recursive\n");
                break;
            case 'f':
                meow.force = true;
                printf("force\n");
                break;
            case 'd':
                meow.dry_run = true;
                printf("dry run\n");
                break;
            default:
                // TODO: this
                fprintf(stderr, "Usage: %s [-hrf]\n", argv[0]);
                return EXIT_FAILURE;
        }
    }

    DEBUGPRINT("args_index: %i\n", iArgsIndex);
    for (int i = 0; i < argc; i++) {
        // compiler will optimize and remove this useless loop
        DEBUGPRINT("argv[%i]: %s\n", i, argv[i]);
    }

    if (iArgsIndex + 1 == argc) {
        fprintf(stderr, "Fatal error: no file(s) specified; no clue what to delete");
        fPrintError(argv[0]);
        return EXIT_FAILURE;
    }

    for (int i = iArgsIndex + 1; i < argc; i++) {
        DEBUGPRINT("index: %i\n", i);
        // if (rt_already_parsed_double_dash && strcmp(argv[i], "--") == 0) {
        if (!bAlreadyParsedDoubleDash && strcmp(argv[i], "--") == 0) {
            bAlreadyParsedDoubleDash = true;
            DEBUGPRINT("skipping -- (first hit)\n");
            // its a --; don't skip it again
            continue;
        }
        if (meow.dry_run) {
            printf("would delete: %s\n", argv[i]); // last element seems to be nah
        } else {
            fTryDeleteFile(argv[i], meow);
            // remove(argv[i]);
        }
    }

    return EXIT_SUCCESS;
}

