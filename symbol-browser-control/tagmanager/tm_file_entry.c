/*
*
*   Copyright (c) 2001-2002, Biswapesh Chattopadhyay
*
*   This source code is released for free distribution under the terms of the
*   GNU General Public License.
*
*/

#include "general.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <fnmatch.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "tm_work_object.h"
#include "tm_file_entry.h"

static GMemChunk *file_mem_chunk = NULL;

#define FILE_NEW(T) {\
	if (!file_mem_chunk) \
		file_mem_chunk = g_mem_chunk_new("TMFileEntry MemChunk", sizeof(TMFileEntry), 1024 \
		  , G_ALLOC_AND_FREE); \
	(T) = g_chunk_new0(TMFileEntry, file_mem_chunk);}

#define FILE_FREE(T) g_mem_chunk_free(file_mem_chunk, (T))

void tm_file_entry_print(TMFileEntry *entry, gpointer __unused__ user_data
  , guint level)
{
	guint i;

	g_return_if_fail(entry);
	for (i=0; i < level; ++i)
		fputc('\t', stderr);
	fprintf(stderr, "%s\n", entry->name);
}

gint tm_file_entry_compare(TMFileEntry *e1, TMFileEntry *e2)
{
	g_return_val_if_fail(e1 && e2 && e1->name && e2->name, 0);
#ifdef DEBUG
	g_message("Comparing %s and %s", e1->name, e2->name);
#endif
	return strcmp(e1->name, e2->name);
}

static TMFileType tm_file_entry_type(const char *path)
{
	struct stat s;

	if (0 != stat(path, &s))
		return tm_file_unknown_t;
	if S_ISDIR(s.st_mode)
		return tm_file_dir_t;
	else if S_ISREG(s.st_mode)
		return tm_file_regular_t;
	else
		return tm_file_unknown_t;
}

TMFileEntry *tm_file_entry_new(const char *path, TMFileEntry *parent
  , gboolean recurse, const char **match, const char **ignore
  , gboolean ignore_hidden)
{
	TMFileEntry *entry;
	const char **t_match;
	const char **t_ignore;
	char *real_path;

	g_assert(path);
	real_path = tm_get_real_path(path);
	FILE_NEW(entry);
	entry->type = tm_file_entry_type(real_path);
	if (tm_file_unknown_t == entry->type)
	{
#ifdef DEBUG
		g_warning("Unknown file: %s", path);
#endif
		g_free(real_path);
		FILE_FREE(entry);
		return NULL;
	}
	entry->parent = parent;
	entry->children = NULL;
	entry->path = real_path;
	entry->name = strrchr(entry->path, '/');
	if (entry->name)
		++ (entry->name);
	else
		entry->name = entry->path;
	if (parent && match && (tm_file_regular_t == entry->type))
	{
		gboolean matched = FALSE;
		for (t_match = match; (*t_match); ++ t_match)
		{
			if (0 == fnmatch(*t_match, entry->name, 0))
			{
				matched = TRUE;
				break;
			}
		}
		if (!matched)
		{
#ifdef DEBUG
			g_warning("%s did not match list. Removing..", entry->path);
#endif
			tm_file_entry_free(entry);
			return NULL;
		}
	}
	if (parent && ignore)
	{
		gboolean ignored = FALSE;
		for (t_ignore = ignore; (*t_ignore); ++ t_ignore)
		{
			if (0 == fnmatch(*t_ignore, entry->name, 0))
			{
				ignored = TRUE;
				break;
			}
		}
		if (ignored)
		{
#ifdef DEBUG
			g_warning("%s matched %s. Ignoring..", entry->path, *ignore);
#endif
			tm_file_entry_free(entry);
			return NULL;
		}
	}
	if (('.' == entry->name[0]) && ignore_hidden && parent)
	{
#ifdef DEBUG
		g_warning("Ignoring hidden file %s", entry->path);
#endif
		tm_file_entry_free(entry);
		return NULL;
	}
	if ((tm_file_dir_t == entry->type) && recurse)
	{
		DIR *dir;
		struct dirent *dir_entry;
		TMFileEntry *new_entry;
		char file_name[PATH_MAX];

#ifdef DEBUG
		g_message("Recursing into %s", entry->path);
#endif
		if (NULL != (dir = opendir(entry->path)))
		{
			while (NULL != (dir_entry = readdir(dir)))
			{
				if ((0 != strcmp(dir_entry->d_name, "."))
					&& (0 != strcmp(dir_entry->d_name, "..")))
				{
					g_snprintf(file_name, PATH_MAX, "%s/%s", entry->path
					  , dir_entry->d_name);
					new_entry = tm_file_entry_new(file_name, entry, TRUE, match
					  , ignore, ignore_hidden);
					if (new_entry)
						entry->children = g_slist_prepend(entry->children, new_entry);
				}
			}
			closedir(dir);
			entry->children = g_slist_sort(entry->children, (GCompareFunc) tm_file_entry_compare);
		}
	}
	return entry;
}

void tm_file_entry_free(gpointer entry)
{
	if (entry)
	{
		TMFileEntry *file_entry = TM_FILE_ENTRY(entry);
		if (file_entry->children)
		{
			GSList *tmp;
			for (tmp = file_entry->children; tmp; tmp = g_slist_next(tmp))
				tm_file_entry_free(tmp->data);
			g_slist_free(file_entry->children);
		}
		g_free(file_entry->path);
		FILE_FREE(file_entry);
	}
}

void tm_file_entry_foreach(TMFileEntry *entry, TMFileEntryFunc func
  , gpointer user_data, guint level, gboolean reverse)
{
	g_assert(entry);
	g_assert(func);

	if ((reverse) && (entry->children))
	{
		GSList *tmp;
		for (tmp = entry->children; tmp; tmp = g_slist_next(tmp))
			tm_file_entry_foreach(TM_FILE_ENTRY(tmp->data), func
			  , user_data, level + 1, TRUE);
	}
	func(entry, user_data, level);
	if ((!reverse) && (entry->children))
	{
		GSList *tmp;
		for (tmp = entry->children; tmp; tmp = g_slist_next(tmp))
			tm_file_entry_foreach(TM_FILE_ENTRY(tmp->data), func
			  , user_data, level + 1, FALSE);
	}
}
