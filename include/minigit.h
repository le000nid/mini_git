#ifndef MINIGIT_H
#define MINIGIT_H

int minigit_init(void);
int minigit_add(const char *path);
int minigit_rm(const char *path);
int minigit_commit(const char *message);
int minigit_log(void);
int minigit_files(const char *commit_id);
int minigit_show(const char *commit_id, const char *path);
int minigit_exists(const char *commit_id, const char *path);
int minigit_print_commit(const char *commit_id);
int minigit_stats(void);
int minigit_status(void);

#endif