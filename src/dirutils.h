#include <stdio.h>	// size_t
#include <sys/types.h>	// mode_t

#ifndef KAULMATE_DIRUTILS_H
#define KAULMATE_DIRUTILS_H

int config_dir(char *dir, size_t len, const char *basename);
int dir_concat(char *dir, size_t len, const char *basename);
int dir_ensure(const char *dir, const mode_t *mode);
int dir_exists(const char *dir);

#endif // KAULMATE_DIRUTILS_H
