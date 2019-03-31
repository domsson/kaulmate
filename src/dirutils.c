#include "dirutils.h"
#include <stdio.h>	// snprintf(), size_t
#include <stdlib.h>	// getenv()
#include <string.h>	// strlen()
#include <errno.h>	// errno
#include <sys/stat.h>	// stat(), mkdir()
#include <sys/types.h>	// mode_t

/*
 * Writes the user's config path in the buffer supplied with `dir`.
 * Tries to query the environment variable `XDF_CONFIG_HOME`, if set.
 * Otherwise, the user's home directory plus `/.config` will be used.
 * If basename is given, it is assumed to be a directory within the
 * user's config directory and will therefore be added to the path. 
 * This function does not check whether the returned path exists.
 * The size of `dir` should be given in `len` (PATH_MAX recommended).
 * Returns the string length of `dir` on success. If the buffer was
 * too small or some other error occured, 0 will be returned.
 */
int config_dir(char *dir, size_t len, const char *basename)
{
	// Make space for the return value
	int n;
	
	// Query the user's config dir - this might not be set though
	char *cfg_home = getenv("XDF_CONFIG_HOME");
	
	// If XDF_CONFIG_HOME was set, we'll use that
	if (cfg_home)
	{
		n = snprintf(dir, len, "%s/%s", cfg_home, basename ? basename : "");
	}
	
	// Use "$HOME/.config", as that's what's mostly in use these days
	n = snprintf(dir, len, "%s/%s/%s", getenv("HOME"), ".config", basename ? basename : "");

	// Check if snprintf errord on us
	if (n < 0)
	{
		return 0;
	}

	// The config path did not fit into dir - now it is truncated	
	if ((size_t) n >= len)
	{
		return 0;
	}

	// Some other error occured with snprintf
	if (n < 0)
	{
		return 0;
	}

	// All good, we return strlen(dir) as returned by snprintf()
	return n;
}

/*
 * Adds `basename` to the end of `dir`, separated with a slash.
 * The buffer size of `dir` needs to be provided in `len`. The buffer
 * needs to be sufficiently big to contain the concatenated path.
 * If the concatenated path can not fit in `dir`, then `dir` will
 * not be mofidied and 0 will be returned. 
 * On success, the new string length of `dir` will be returned.
 * If `basename` is not null-terminated, the behavior is undefined.
 */
int dir_concat(char *dir, size_t len, const char *basename)
{
	// Figure out the occupied part of the dir buffer
	size_t dir_len = strnlen(dir, len);

	// Get the string length of the basename
	size_t base_len = strlen(basename);

	// If basename is empty, we're already done
	if (base_len == 0)
	{
		return dir_len;
	}
	
	// Check if the dir string is empty
	int dir_empty = (dir_len == 0);

	// Check if there is already a slash at the end of dir
	int path_sep_missing = !(dir[dir_len - 1] == '/');
	
	// Calculate the required length without null terminator
	size_t req_len = dir_empty + dir_len + path_sep_missing + base_len;

	// Can everything, including null terminator, fit in dir?
	if (req_len + 1 > len)
	{
		return 0;
	}

	// If dir is empty, we make it the current dir
	if (dir_empty)
	{
		dir[dir_len++] = '.';
	}

	// Add the path separator, if none is present
	if (path_sep_missing)
	{
		dir[dir_len++] = '/';
	}

	// Run through basename and add each char to dir
	for (size_t i = 0; i < base_len; ++i)
	{
		dir[dir_len++] = basename[i];
	}

	// Add the null terminator to the very end
	dir[dir_len] = '\0';
	
	return dir_len;
}

/*
 * Checks if the directory `dir` exists. If not, attempts to create it.
 * If the directory is created, the permissions given in `mode` will be
 * used, unless `mode` is NULL, in which case rwxr-xr-x will be used.
 * Returns 1 on success (dir existed or has been created), 0 on error.
 * If an error occured, errno will be set accordingly by mkdir().
 */
int dir_ensure(const char *dir, const mode_t *mode)
{
	// Define the default access permissions we want to use
	mode_t default_mode = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;

	// Try to create the dir with the given (or default) mode
	int mkdir_err = mkdir(dir, (mode ? *mode : default_mode));

	// mkdir() was able to create the directory, we're done
	if (mkdir_err == 0)
	{
		return 1;
	}

	// mkdir() failed because the dir already exists, we're done
	if (errno == EEXIST)
	{
		return 1;
	}

	// Some other error occured, we failed
	return 0;
}

/*
 * Checks if the given directory exists. Returns 1 if so, otherwise 0.
 * Will also return 0 if the path exists, but is not a directory, or if
 * some other error occured, in which case errno will be set by stat().
 */
int dir_exists(const char *dir)
{
	struct stat s;
	
	// An error occured (errno should be set)
	if (stat(dir, &s) == -1)
	{
		return 0;
	}

	// stat() confirms dir to be a directory
	if (S_ISDIR(s.st_mode))
	{
		return 1;
	}

	// No error, but dir doesn't look like a dir
	return 0;
}

