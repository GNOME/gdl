/*
*
*   Copyright (c) 2001-2002, Biswapesh Chattopadhyay
*
*   This source code is released for free distribution under the terms of the
*   GNU General Public License.
*
*/

#ifndef TM_FILE_ENTRY_H
#define TM_FILE_ENTRY_H

#include <glib.h>

/*! \file
The TMFileEntry structure and associated functions can be used
for file and directory traversal. The following example demonstrates
the use of TMFileEntry.
\include tm_file_tree_dump.c
*/

#ifdef __cplusplus
extern "C"
{
#endif

/*! Enum defining file types */
typedef enum
{
	tm_file_unknown_t, /*!< Unknown file type/file does not exist */
	tm_file_regular_t, /*!< Is a regular file */
	tm_file_dir_t /*!< Is a directory */
} TMFileType;

/*!
 This example demonstrates the use of TMFileEntry and associated functions
 for managing file hierarchies in a project.

 \example tm_file_tree_dump.c
*/

/*! This structure stores the file tree */
typedef struct _TMFileEntry
{
	TMFileType type; /*!< File type */
	char *path; /*!< Full path to the file (incl. dir and name) */
	char *name; /*!< Just the file name (path minus the directory) */
	struct _TMFileEntry *parent; /*!< The parent directory file entry */
	GSList *children; /*!< List of children (for directory) */
} TMFileEntry;

/*! Prototype for the function that gets called for each entry when
 tm_file_entry_foreach() is called.
*/
typedef void (*TMFileEntryFunc) (TMFileEntry *entry, gpointer user_data
  , guint level);

/*! Convinience casting macro */
#define TM_FILE_ENTRY(E) ((TMFileEntry *) (E))

/*! Function that compares two file entries on name and returns the
 difference
*/
gint tm_file_entry_compare(TMFileEntry *e1, TMFileEntry *e2);

/*! Function to create a new file entry structure.
\param path Path to the file for which the entry is to be created.
\param parent SHould be NULL for the first call. Since the function calls
 itself recursively, this parameter is required to build the hierarchy.
\param recurse Whether the entry is to be recursively scanned (for
 directories only)
\param match Null terminated list of file names to match. If set to NULL,
 all files match. You can use wildcards like '*.c'. See the example program
 for usage. Note that match list is only applied to files, not directories.
\param ignore Opposite of match. All files matching any of the patterns
 supplied are ignored. If set to NULL, no file is ignored. Should be a NULL
 terminated list. See the example for usage. As opposed to match list, ignore
 list is applied to files as well as directories.
\param ignore_hidden If set to TRUE, hidden files (files starting with '.')
 are ignored.
\return Populated TMFileEntry structure on success, NULL on failure.
*/
TMFileEntry *tm_file_entry_new(const char *path, TMFileEntry *parent
  , gboolean recurse, const char **match, const char **ignore
  , gboolean ignore_hidden);

/*! Frees a TMFileEntry structure. Freeing is recursive, so all child
 entries are freed as well.
\param entry The TMFileEntry structure to be freed.
*/
void tm_file_entry_free(gpointer entry);

/*! This will call the function func() for each file entry.
\param entry The root file entry.
\param func The function to be called.
\param user_data Extra information to be passed to the function.
\param level The recursion level. You should set this to 0 initially.
\param reverse If set to TRUE, traversal is in reverse hierarchical order
*/
void tm_file_entry_foreach(TMFileEntry *entry, TMFileEntryFunc func
  , gpointer user_data, guint level, gboolean reverse);

/*! This is a sample function to show the use of tm_file_entry_foreach().
*/
void tm_file_entry_print(TMFileEntry *entry, gpointer user_data, guint level);

#ifdef __cplusplus
}
#endif

#endif /* TM_FILE_ENTRY_H */
