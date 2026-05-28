#include "minigit.h"
#include "hash.h"

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#define MINIGIT_DIR ".minigit"
#define OBJECTS_DIR ".minigit/objects"
#define COMMITS_DIR ".minigit/commits"
#define HEAD_FILE ".minigit/HEAD"
#define INDEX_FILE ".minigit/index"

#define PATH_BUFFER_SIZE 512

static int path_exists(const char *path)
{
    struct stat st;

    return stat(path, &st) == 0;
}

static int create_directory(const char *path)
{
    if (mkdir(path, 0755) == 0) {
        return 0;
    }

    if (errno == EEXIST) {
        return 0;
    }

    fprintf(stderr, "Error: cannot create directory '%s': %s\n",
            path,
            strerror(errno));

    return 1;
}

static int create_file_with_content(const char *path, const char *content)
{
    FILE *file;

    file = fopen(path, "w");
    if (file == NULL) {
        fprintf(stderr, "Error: cannot create file '%s': %s\n",
                path,
                strerror(errno));
        return 1;
    }

    if (content != NULL) {
        if (fputs(content, file) == EOF) {
            fprintf(stderr, "Error: cannot write to file '%s'\n", path);
            fclose(file);
            return 1;
        }
    }

    if (fclose(file) != 0) {
        fprintf(stderr, "Error: cannot close file '%s': %s\n",
                path,
                strerror(errno));
        return 1;
    }

    return 0;
}

static int ensure_repository_exists(void)
{
    if (!path_exists(MINIGIT_DIR)) {
        fprintf(stderr, "Error: not a Mini-Git repository. Run './minigit init' first.\n");
        return 1;
    }

    return 0;
}

static int copy_file(const char *source_path, const char *destination_path)
{
    FILE *source;
    FILE *destination;
    int ch;

    source = fopen(source_path, "rb");
    if (source == NULL) {
        fprintf(stderr, "Error: cannot open file '%s': %s\n",
                source_path,
                strerror(errno));
        return 1;
    }

    destination = fopen(destination_path, "wb");
    if (destination == NULL) {
        fprintf(stderr, "Error: cannot create object '%s': %s\n",
                destination_path,
                strerror(errno));
        fclose(source);
        return 1;
    }

    while ((ch = fgetc(source)) != EOF) {
        if (fputc(ch, destination) == EOF) {
            fprintf(stderr, "Error: cannot write object '%s'\n", destination_path);
            fclose(source);
            fclose(destination);
            return 1;
        }
    }

    if (ferror(source)) {
        fprintf(stderr, "Error: cannot read file '%s'\n", source_path);
        fclose(source);
        fclose(destination);
        return 1;
    }

    if (fclose(source) != 0) {
        fprintf(stderr, "Error: cannot close file '%s': %s\n",
                source_path,
                strerror(errno));
        fclose(destination);
        return 1;
    }

    if (fclose(destination) != 0) {
        fprintf(stderr, "Error: cannot close object '%s': %s\n",
                destination_path,
                strerror(errno));
        return 1;
    }

    return 0;
}

static int append_add_to_index(const char *path, uint64_t hash)
{
    FILE *index_file;

    index_file = fopen(INDEX_FILE, "a");
    if (index_file == NULL) {
        fprintf(stderr, "Error: cannot open index: %s\n", strerror(errno));
        return 1;
    }

    if (fprintf(index_file, "A %s %016" PRIx64 "\n", path, hash) < 0) {
        fprintf(stderr, "Error: cannot write to index\n");
        fclose(index_file);
        return 1;
    }

    if (fclose(index_file) != 0) {
        fprintf(stderr, "Error: cannot close index: %s\n", strerror(errno));
        return 1;
    }

    return 0;
}

static int append_delete_to_index(const char *path)
{
    FILE *index_file;

    index_file = fopen(INDEX_FILE, "a");
    if (index_file == NULL) {
        fprintf(stderr, "Error: cannot open index: %s\n", strerror(errno));
        return 1;
    }

    if (fprintf(index_file, "D %s\n", path) < 0) {
        fprintf(stderr, "Error: cannot write to index\n");
        fclose(index_file);
        return 1;
    }

    if (fclose(index_file) != 0) {
        fprintf(stderr, "Error: cannot close index: %s\n", strerror(errno));
        return 1;
    }

    return 0;
}

static int read_head(int *head)
{
    FILE *file;

    if (head == NULL) {
        return 1;
    }

    file = fopen(HEAD_FILE, "r");
    if (file == NULL) {
        fprintf(stderr, "Error: cannot open HEAD: %s\n", strerror(errno));
        return 1;
    }

    if (fscanf(file, "%d", head) != 1) {
        fprintf(stderr, "Error: invalid HEAD file\n");
        fclose(file);
        return 1;
    }

    if (fclose(file) != 0) {
        fprintf(stderr, "Error: cannot close HEAD: %s\n", strerror(errno));
        return 1;
    }

    return 0;
}

static int write_head(int head)
{
    FILE *file;

    file = fopen(HEAD_FILE, "w");
    if (file == NULL) {
        fprintf(stderr, "Error: cannot open HEAD for writing: %s\n", strerror(errno));
        return 1;
    }

    if (fprintf(file, "%d\n", head) < 0) {
        fprintf(stderr, "Error: cannot write HEAD\n");
        fclose(file);
        return 1;
    }

    if (fclose(file) != 0) {
        fprintf(stderr, "Error: cannot close HEAD: %s\n", strerror(errno));
        return 1;
    }

    return 0;
}

static int is_index_empty(void)
{
    FILE *file;
    int ch;

    file = fopen(INDEX_FILE, "r");
    if (file == NULL) {
        return 1;
    }

    ch = fgetc(file);
    fclose(file);

    return ch == EOF;
}

static int clear_index(void)
{
    FILE *file;

    file = fopen(INDEX_FILE, "w");
    if (file == NULL) {
        fprintf(stderr, "Error: cannot clear index: %s\n", strerror(errno));
        return 1;
    }

    if (fclose(file) != 0) {
        fprintf(stderr, "Error: cannot close index: %s\n", strerror(errno));
        return 1;
    }

    return 0;
}

static int make_commit_path(int commit_id, char *buffer, size_t buffer_size)
{
    int written;

    written = snprintf(buffer,
                       buffer_size,
                       "%s/%d.commit",
                       COMMITS_DIR,
                       commit_id);

    if (written < 0 || written >= (int)buffer_size) {
        fprintf(stderr, "Error: commit path is too long\n");
        return 1;
    }

    return 0;
}

static int copy_index_to_commit(FILE *commit_file)
{
    FILE *index_file;
    char line[1024];
    char action;
    char file_path[512];
    char file_hash[128];

    index_file = fopen(INDEX_FILE, "r");
    if (index_file == NULL) {
        fprintf(stderr, "Error: cannot open index: %s\n", strerror(errno));
        return 1;
    }

    while (fgets(line, sizeof(line), index_file) != NULL) {
        if (sscanf(line, " %c %511s %127s", &action, file_path, file_hash) >= 2) {
            if (action == 'A') {
                if (fprintf(commit_file, "%s %s\n", file_path, file_hash) < 0) {
                    fprintf(stderr, "Error: cannot write file entry to commit\n");
                    fclose(index_file);
                    return 1;
                }
            }
        }
    }

    if (ferror(index_file)) {
        fprintf(stderr, "Error: cannot read index\n");
        fclose(index_file);
        return 1;
    }

    if (fclose(index_file) != 0) {
        fprintf(stderr, "Error: cannot close index: %s\n", strerror(errno));
        return 1;
    }

    return 0;
}

static int read_commit_header(int commit_id,
                              int *id,
                              int *parent,
                              char *message,
                              size_t message_size)
{
    char commit_path[PATH_BUFFER_SIZE];
    FILE *commit_file;
    char line[1024];

    if (id == NULL || parent == NULL || message == NULL) {
        return 1;
    }

    if (make_commit_path(commit_id, commit_path, sizeof(commit_path)) != 0) {
        return 1;
    }

    commit_file = fopen(commit_path, "r");
    if (commit_file == NULL) {
        fprintf(stderr, "Error: cannot open commit '%s': %s\n",
                commit_path,
                strerror(errno));
        return 1;
    }

    if (fscanf(commit_file, "id %d\n", id) != 1) {
        fprintf(stderr, "Error: invalid commit id in '%s'\n", commit_path);
        fclose(commit_file);
        return 1;
    }

    if (fscanf(commit_file, "parent %d\n", parent) != 1) {
        fprintf(stderr, "Error: invalid parent in '%s'\n", commit_path);
        fclose(commit_file);
        return 1;
    }

    if (fgets(line, sizeof(line), commit_file) == NULL) {
        fprintf(stderr, "Error: invalid message in '%s'\n", commit_path);
        fclose(commit_file);
        return 1;
    }

    if (strncmp(line, "message ", 8) != 0) {
        fprintf(stderr, "Error: invalid message line in '%s'\n", commit_path);
        fclose(commit_file);
        return 1;
    }

    if (snprintf(message, message_size, "%s", line + 8) >= (int)message_size) {
        fprintf(stderr, "Error: commit message is too long\n");
        fclose(commit_file);
        return 1;
    }

    message[strcspn(message, "\n")] = '\0';

    if (fclose(commit_file) != 0) {
        fprintf(stderr, "Error: cannot close commit '%s': %s\n",
                commit_path,
                strerror(errno));
        return 1;
    }

    return 0;
}

static int print_commit_files(int commit_id)
{
    char commit_path[PATH_BUFFER_SIZE];
    FILE *commit_file;
    char line[1024];
    int in_files_section;
    int has_files;

    if (make_commit_path(commit_id, commit_path, sizeof(commit_path)) != 0) {
        return 1;
    }

    commit_file = fopen(commit_path, "r");
    if (commit_file == NULL) {
        fprintf(stderr, "Error: cannot open commit '%s': %s\n",
                commit_path,
                strerror(errno));
        return 1;
    }

    in_files_section = 0;
    has_files = 0;

    while (fgets(line, sizeof(line), commit_file) != NULL) {
        line[strcspn(line, "\n")] = '\0';

        if (strcmp(line, "files") == 0) {
            in_files_section = 1;
            continue;
        }

        if (strcmp(line, "end") == 0) {
            break;
        }

        if (in_files_section) {
            has_files = 1;
            printf("  %s\n", line);
        }
    }

    if (ferror(commit_file)) {
        fprintf(stderr, "Error: cannot read commit '%s'\n", commit_path);
        fclose(commit_file);
        return 1;
    }

    if (!has_files) {
        printf("  no files\n");
    }

    if (fclose(commit_file) != 0) {
        fprintf(stderr, "Error: cannot close commit '%s': %s\n",
                commit_path,
                strerror(errno));
        return 1;
    }

    return 0;
}

static int find_file_hash_in_commit(int commit_id,
                                    const char *path,
                                    char *hash,
                                    size_t hash_size)
{
    char commit_path[PATH_BUFFER_SIZE];
    FILE *commit_file;
    char line[1024];
    char file_path[512];
    char file_hash[128];
    int in_files_section;

    if (path == NULL || hash == NULL) {
        return 1;
    }

    if (make_commit_path(commit_id, commit_path, sizeof(commit_path)) != 0) {
        return 1;
    }

    commit_file = fopen(commit_path, "r");
    if (commit_file == NULL) {
        fprintf(stderr, "Error: cannot open commit '%s': %s\n",
                commit_path,
                strerror(errno));
        return 1;
    }

    in_files_section = 0;

    while (fgets(line, sizeof(line), commit_file) != NULL) {
        line[strcspn(line, "\n")] = '\0';

        if (strcmp(line, "files") == 0) {
            in_files_section = 1;
            continue;
        }

        if (strcmp(line, "end") == 0) {
            break;
        }

        if (in_files_section) {
            if (sscanf(line, "%511s %127s", file_path, file_hash) == 2) {
                if (strcmp(file_path, path) == 0) {
                    if (snprintf(hash, hash_size, "%s", file_hash) >=
                        (int)hash_size) {
                        fprintf(stderr, "Error: hash buffer is too small\n");
                        fclose(commit_file);
                        return 1;
                    }

                    if (fclose(commit_file) != 0) {
                        fprintf(stderr, "Error: cannot close commit '%s': %s\n",
                                commit_path,
                                strerror(errno));
                        return 1;
                    }

                    return 0;
                }
            }
        }
    }

    if (ferror(commit_file)) {
        fprintf(stderr, "Error: cannot read commit '%s'\n", commit_path);
        fclose(commit_file);
        return 1;
    }

    if (fclose(commit_file) != 0) {
        fprintf(stderr, "Error: cannot close commit '%s': %s\n",
                commit_path,
                strerror(errno));
        return 1;
    }

    return 2;
}

static int print_object_by_hash(const char *hash)
{
    char object_path[PATH_BUFFER_SIZE];
    FILE *object_file;
    int ch;

    if (hash == NULL) {
        return 1;
    }

    if (snprintf(object_path,
                 sizeof(object_path),
                 "%s/%s.obj",
                 OBJECTS_DIR,
                 hash) >= (int)sizeof(object_path)) {
        fprintf(stderr, "Error: object path is too long\n");
        return 1;
    }

    object_file = fopen(object_path, "rb");
    if (object_file == NULL) {
        fprintf(stderr, "Error: cannot open object '%s': %s\n",
                object_path,
                strerror(errno));
        return 1;
    }

    while ((ch = fgetc(object_file)) != EOF) {
        if (putchar(ch) == EOF) {
            fprintf(stderr, "Error: cannot write output\n");
            fclose(object_file);
            return 1;
        }
    }

    if (ferror(object_file)) {
        fprintf(stderr, "Error: cannot read object '%s'\n", object_path);
        fclose(object_file);
        return 1;
    }

    if (fclose(object_file) != 0) {
        fprintf(stderr, "Error: cannot close object '%s': %s\n",
                object_path,
                strerror(errno));
        return 1;
    }

    return 0;
}

static int index_contains_path(const char *path)
{
    FILE *index_file;
    char line[1024];
    char action;
    char file_path[512];
    char file_hash[128];

    index_file = fopen(INDEX_FILE, "r");
    if (index_file == NULL) {
        return 0;
    }

    while (fgets(line, sizeof(line), index_file) != NULL) {
        if (sscanf(line, " %c %511s %127s", &action, file_path, file_hash) >= 2) {
            if (strcmp(file_path, path) == 0) {
                fclose(index_file);
                return 1;
            }
        }
    }

    fclose(index_file);
    return 0;
}

static int copy_previous_commit_files(FILE *commit_file, int parent_id)
{
    char parent_path[PATH_BUFFER_SIZE];
    FILE *parent_file;
    char line[1024];
    char file_path[512];
    char file_hash[128];
    int in_files_section;

    if (parent_id == 0) {
        return 0;
    }

    if (make_commit_path(parent_id, parent_path, sizeof(parent_path)) != 0) {
        return 1;
    }

    parent_file = fopen(parent_path, "r");
    if (parent_file == NULL) {
        fprintf(stderr, "Error: cannot open parent commit '%s': %s\n",
                parent_path,
                strerror(errno));
        return 1;
    }

    in_files_section = 0;

    while (fgets(line, sizeof(line), parent_file) != NULL) {
        line[strcspn(line, "\n")] = '\0';

        if (strcmp(line, "files") == 0) {
            in_files_section = 1;
            continue;
        }

        if (strcmp(line, "end") == 0) {
            break;
        }

        if (in_files_section) {
            if (sscanf(line, "%511s %127s", file_path, file_hash) == 2) {
                if (!index_contains_path(file_path)) {
                    if (fprintf(commit_file, "%s %s\n", file_path, file_hash) < 0) {
                        fprintf(stderr, "Error: cannot copy parent file entry\n");
                        fclose(parent_file);
                        return 1;
                    }
                }
            }
        }
    }

    if (ferror(parent_file)) {
        fprintf(stderr, "Error: cannot read parent commit\n");
        fclose(parent_file);
        return 1;
    }

    if (fclose(parent_file) != 0) {
        fprintf(stderr, "Error: cannot close parent commit: %s\n", strerror(errno));
        return 1;
    }

    return 0;
}

int minigit_init(void)
{
    if (path_exists(MINIGIT_DIR)) {
        printf("Mini-Git repository already exists in %s\n", MINIGIT_DIR);
        return 0;
    }

    if (create_directory(MINIGIT_DIR) != 0) {
        return 1;
    }

    if (create_directory(OBJECTS_DIR) != 0) {
        return 1;
    }

    if (create_directory(COMMITS_DIR) != 0) {
        return 1;
    }

    if (create_file_with_content(HEAD_FILE, "0\n") != 0) {
        return 1;
    }

    if (create_file_with_content(INDEX_FILE, "") != 0) {
        return 1;
    }

    printf("Initialized empty Mini-Git repository in %s\n", MINIGIT_DIR);

    return 0;
}

int minigit_add(const char *path)
{
    uint64_t hash;
    char object_path[PATH_BUFFER_SIZE];

    if (ensure_repository_exists() != 0) {
        return 1;
    }

    if (!path_exists(path)) {
        fprintf(stderr, "Error: file '%s' does not exist\n", path);
        return 1;
    }

    if (hash_file(path, &hash) != 0) {
        return 1;
    }

    if (snprintf(object_path,
                 sizeof(object_path),
                 "%s/%016" PRIx64 ".obj",
                 OBJECTS_DIR,
                 hash) >= (int)sizeof(object_path)) {
        fprintf(stderr, "Error: object path is too long\n");
        return 1;
    }

    if (!path_exists(object_path)) {
        if (copy_file(path, object_path) != 0) {
            return 1;
        }
    }

    if (append_add_to_index(path, hash) != 0) {
        return 1;
    }

    printf("Added '%s' as object %016" PRIx64 "\n", path, hash);

    return 0;
}

int minigit_rm(const char *path)
{
    if (ensure_repository_exists() != 0) {
        return 1;
    }

    if (path == NULL || path[0] == '\0') {
        fprintf(stderr, "Error: rm requires file path\n");
        return 1;
    }

    if (append_delete_to_index(path) != 0) {
        return 1;
    }

    printf("Marked '%s' for removal\n", path);

    return 0;
}

int minigit_commit(const char *message)
{
    int current_head;
    int new_commit_id;
    char commit_path[PATH_BUFFER_SIZE];
    FILE *commit_file;

    if (ensure_repository_exists() != 0) {
        return 1;
    }

    if (message == NULL || message[0] == '\0') {
        fprintf(stderr, "Error: commit message is empty\n");
        return 1;
    }

    if (is_index_empty()) {
        printf("Nothing to commit\n");
        return 0;
    }

    if (read_head(&current_head) != 0) {
        return 1;
    }

    new_commit_id = current_head + 1;

    if (make_commit_path(new_commit_id, commit_path, sizeof(commit_path)) != 0) {
        return 1;
    }

    commit_file = fopen(commit_path, "w");
    if (commit_file == NULL) {
        fprintf(stderr, "Error: cannot create commit '%s': %s\n",
                commit_path,
                strerror(errno));
        return 1;
    }

    if (fprintf(commit_file, "id %d\n", new_commit_id) < 0 ||
        fprintf(commit_file, "parent %d\n", current_head) < 0 ||
        fprintf(commit_file, "message %s\n", message) < 0 ||
        fprintf(commit_file, "files\n") < 0) {
        fprintf(stderr, "Error: cannot write commit header\n");
        fclose(commit_file);
        return 1;
    }

    if (copy_previous_commit_files(commit_file, current_head) != 0) {
        fclose(commit_file);
        return 1;
    }

    if (copy_index_to_commit(commit_file) != 0) {
        fclose(commit_file);
        return 1;
    }

    if (fprintf(commit_file, "end\n") < 0) {
        fprintf(stderr, "Error: cannot write commit footer\n");
        fclose(commit_file);
        return 1;
    }

    if (fclose(commit_file) != 0) {
        fprintf(stderr, "Error: cannot close commit file: %s\n", strerror(errno));
        return 1;
    }

    if (write_head(new_commit_id) != 0) {
        return 1;
    }

    if (clear_index() != 0) {
        return 1;
    }

    printf("Created commit %d: %s\n", new_commit_id, message);

    return 0;
}

int minigit_log(void)
{
    int current_commit;
    int id;
    int parent;
    char message[1024];

    if (ensure_repository_exists() != 0) {
        return 1;
    }

    if (read_head(&current_commit) != 0) {
        return 1;
    }

    if (current_commit == 0) {
        printf("No commits yet\n");
        return 0;
    }

    while (current_commit != 0) {
        if (read_commit_header(current_commit,
                               &id,
                               &parent,
                               message,
                               sizeof(message)) != 0) {
            return 1;
        }

        printf("commit %d\n", id);
        printf("parent %d\n", parent);
        printf("message %s\n\n", message);

        current_commit = parent;
    }

    return 0;
}

int minigit_files(void)
{
    int current_head;

    if (ensure_repository_exists() != 0) {
        return 1;
    }

    if (read_head(&current_head) != 0) {
        return 1;
    }

    if (current_head == 0) {
        printf("No commits yet\n");
        return 0;
    }

    printf("Files in commit %d:\n", current_head);

    return print_commit_files(current_head);
}

int minigit_show(const char *commit_id, const char *path)
{
    int id;
    int result;
    char hash[128];
    char extra;

    if (ensure_repository_exists() != 0) {
        return 1;
    }

    if (commit_id == NULL || path == NULL) {
        fprintf(stderr, "Error: show requires commit id and file path\n");
        return 1;
    }

    if (sscanf(commit_id, "%d%c", &id, &extra) != 1 || id <= 0) {
        fprintf(stderr, "Error: invalid commit id '%s'\n", commit_id);
        return 1;
    }

    result = find_file_hash_in_commit(id, path, hash, sizeof(hash));

    if (result == 2) {
        fprintf(stderr, "Error: file '%s' not found in commit %d\n", path, id);
        return 1;
    }

    if (result != 0) {
        return 1;
    }

    return print_object_by_hash(hash);
}

int minigit_exists(const char *commit_id, const char *path)
{
    int id;
    int result;
    char hash[128];
    char extra;

    if (ensure_repository_exists() != 0) {
        return 1;
    }

    if (commit_id == NULL || path == NULL) {
        fprintf(stderr, "Error: exists requires commit id and file path\n");
        return 1;
    }

    if (sscanf(commit_id, "%d%c", &id, &extra) != 1 || id <= 0) {
        fprintf(stderr, "Error: invalid commit id '%s'\n", commit_id);
        return 1;
    }

    result = find_file_hash_in_commit(id, path, hash, sizeof(hash));

    if (result == 0) {
        printf("true\n");
        return 0;
    }

    if (result == 2) {
        printf("false\n");
        return 0;
    }

    return 1;
}

int minigit_print_commit(const char *commit_id)
{
    int id;
    int real_id;
    int parent;
    char message[1024];
    char extra;

    if (ensure_repository_exists() != 0) {
        return 1;
    }

    if (commit_id == NULL) {
        fprintf(stderr, "Error: commit-info requires commit id\n");
        return 1;
    }

    if (sscanf(commit_id, "%d%c", &id, &extra) != 1 || id <= 0) {
        fprintf(stderr, "Error: invalid commit id '%s'\n", commit_id);
        return 1;
    }

    if (read_commit_header(id, &real_id, &parent, message, sizeof(message)) != 0) {
        return 1;
    }

    printf("commit %d\n", real_id);
    printf("parent %d\n", parent);
    printf("message %s\n", message);
    printf("\n");
    printf("files:\n");

    return print_commit_files(id);
}

int minigit_status(void)
{
    FILE *index_file;
    char line[1024];
    char action;
    char file_path[512];
    char file_hash[128];
    int has_entries;

    if (ensure_repository_exists() != 0) {
        return 1;
    }

    index_file = fopen(INDEX_FILE, "r");
    if (index_file == NULL) {
        fprintf(stderr, "Error: cannot open index: %s\n", strerror(errno));
        return 1;
    }

    has_entries = 0;

    printf("Staged changes:\n");

    while (fgets(line, sizeof(line), index_file) != NULL) {
        if (sscanf(line, " %c %511s %127s", &action, file_path, file_hash) >= 2) {
            has_entries = 1;

            if (action == 'A') {
                printf("  added/modified: %s %s\n", file_path, file_hash);
            } else if (action == 'D') {
                printf("  deleted: %s\n", file_path);
            }
        }
    }

    if (ferror(index_file)) {
        fprintf(stderr, "Error: cannot read index\n");
        fclose(index_file);
        return 1;
    }

    if (!has_entries) {
        printf("  nothing staged\n");
    }

    if (fclose(index_file) != 0) {
        fprintf(stderr, "Error: cannot close index: %s\n", strerror(errno));
        return 1;
    }

    return 0;
}