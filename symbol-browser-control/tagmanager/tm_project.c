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
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fnmatch.h>

#include "tm_tag.h"
#include "tm_workspace.h"
#include "tm_source_file.h"
#include "tm_file_entry.h"
#include "tm_project.h"

#define TM_FILE_NAME ".tm_project.cache"

static const char *s_sources[] = { "*.c" /* C files */
	, "*.C", "*.cpp", "*.cc", "*.cxx" /* C++ files */
	, "*.h", "*.hh", "*.hpp" /* Header files */
#if 0
	, "*.oaf", "*.gob", "*.idl" /* CORBA/Bonobo files */
	, "*.l", "*.y" /* Lex/Yacc files */
	, "*.ui", "*.moc" /* KDE/QT Files */
	, "*.glade" /* UI files */
#endif
	, "*.java", "*.pl", "*.py" /* Other languages */
	, NULL /* Must terminate with NULL */
};

static const char *s_ignore[] = { "\\.*", "CVS", "intl", "po", NULL };

guint project_class_id = 0;

gboolean tm_project_init(TMProject *project, const char *dir
  , const char **sources, const char **ignore, gboolean force)
{
	struct stat s;
	char path[PATH_MAX];

	g_return_val_if_fail((project && dir), FALSE);
#ifdef DEBUG
	g_message("Initializing project %s", dir);
#endif
	if (0 == project_class_id)
	{
		project_class_id = tm_work_object_register(tm_project_free, tm_project_update
		  , tm_project_find_file);
	}

	if ((0 != stat(dir, &s)) || (!S_ISDIR(s.st_mode)))
	{
		g_warning("%s: Not a valid directory", dir);
		return FALSE;
	}
	project->dir = tm_get_real_path(dir);
	if (sources)
		project->sources = sources;
	else
		project->sources = s_sources;
	if (ignore)
		project->ignore = ignore;
	else
		project->ignore = s_ignore;
	project->file_list = NULL;
	g_snprintf(path, PATH_MAX, "%s/%s", project->dir, TM_FILE_NAME);
	if ((0 != stat(path, &s)) || (0 == s.st_size))
		force = TRUE;
	if (FALSE == tm_work_object_init(&(project->work_object),
		  project_class_id, path, force))
	{
		g_warning("Unable to init project file %s", path);
		g_free(project->dir);
		return FALSE;
	}
	tm_workspace_add_object(TM_WORK_OBJECT(project));
	tm_project_open(project, force);
	if (!project->file_list || (0 == project->file_list->len))
		tm_project_autoscan(project);
	return TRUE;
}

TMWorkObject *tm_project_new(const char *dir, const char **sources
  , const char **ignore, gboolean force)
{
	TMProject *project = g_new(TMProject, 1);
	if (FALSE == tm_project_init(project, dir, sources, ignore, force))
	{
		g_free(project);
		return NULL;
	}
	return (TMWorkObject *) project;
}

void tm_project_destroy(TMProject *project)
{
	g_assert(project);
#ifdef DEBUG
	g_message("Destroying project: %s", project->work_object.file_name);
#endif

	if (project->file_list)
	{
		guint i;
		for (i = 0; i < project->file_list->len; ++i)
			tm_source_file_free(project->file_list->pdata[i]);
		g_ptr_array_free(project->file_list, TRUE);
	}
	tm_workspace_remove_object(TM_WORK_OBJECT(project), FALSE);
	g_free(project->dir);
	tm_work_object_destroy(&(project->work_object));
}

void tm_project_free(gpointer project)
{
	if (NULL != project)
	{
		tm_project_destroy(TM_PROJECT(project));
		g_free(project);
	}
}

gboolean tm_project_add_file(TMProject *project, const char *file_name
  ,gboolean update)
{
	TMWorkObject *source_file;
	char *path;
	guint i;

	g_return_val_if_fail((project && file_name), FALSE);
	if ((project->file_list) && (project->file_list->len > 0))
	{
		path = tm_get_real_path(file_name);
		for (i=0; i < project->file_list->len; ++i)
		{
			source_file = TM_WORK_OBJECT(project->file_list->pdata[i]);
			if (source_file)
			{
				if (0 == strcmp(source_file->file_name, path))
				{
					TM_SOURCE_FILE(source_file)->inactive = FALSE;
					g_free(path);
					return TRUE;
				}
			}
		}
		g_free(path);
	}
	if (NULL == (source_file = tm_source_file_new(file_name, TRUE)))
		return FALSE;
	source_file->parent = TM_WORK_OBJECT(project);
	if (NULL == project->file_list)
		project->file_list = g_ptr_array_new();
	g_ptr_array_add(project->file_list, source_file);
	if (update)
		tm_project_update(TM_WORK_OBJECT(project), TRUE, FALSE, TRUE);
	return TRUE;
}

TMWorkObject *tm_project_find_file(TMWorkObject *work_object
  , const char *file_name, gboolean name_only)
{
	TMProject *project;

	g_return_val_if_fail(work_object && file_name, NULL);
	if (!IS_TM_PROJECT(work_object))
	{
		g_warning("Non project pointer passed to tm_project_find_file(%s)", file_name);
		return NULL;
	}
	project = TM_PROJECT(work_object);
	if ((NULL == project->file_list) || (0 == project->file_list->len))
		return NULL;
	else
	{
		guint i;
		char *name, *name1;
		if (name_only)
		{
			name = strrchr(file_name, '/');
			if (name)
				name = g_strdup(name + 1);
			else
				name = g_strdup(file_name);
		}
		else
			name = tm_get_real_path(file_name);
		for (i=0; i < project->file_list->len; ++i)
		{
			if (name_only)
				name1 = TM_WORK_OBJECT(project->file_list->pdata[i])->file_name;
			else
				name1 = TM_WORK_OBJECT(project->file_list->pdata[i])->short_name;
			if (0 == strcmp(name, name1))
			{
				g_free(name);
				return TM_WORK_OBJECT(project->file_list->pdata[i]);
			}
		}
		g_free(name);
	}
	return NULL;
}

gboolean tm_project_remove_object(TMProject *project, TMWorkObject *w)
{
	guint i;

	g_return_val_if_fail((project && w), FALSE);
	if (!project->file_list)
		return FALSE;
	for (i=0; i < project->file_list->len; ++i)
	{
		if (w == project->file_list->pdata[i])
		{
			tm_work_object_free(w);
			g_ptr_array_remove_index(project->file_list, i);
			tm_project_update(TM_WORK_OBJECT(project), TRUE, FALSE, TRUE);
			return TRUE;
		}
	}
	return FALSE;
}

void tm_project_recreate_tags_array(TMProject *project)
{
	guint i, j;
	TMWorkObject *source_file;

	g_return_if_fail(project);
#ifdef DEBUG
	g_message("Recreating tags for project: %s", project->work_object.file_name);
#endif

	if (NULL != project->work_object.tags_array)
		g_ptr_array_set_size(project->work_object.tags_array, 0);
	else
		project->work_object.tags_array = g_ptr_array_new();

	if (!project->file_list)
		return;
	for (i=0; i < project->file_list->len; ++i)
	{
		source_file = TM_WORK_OBJECT(project->file_list->pdata[i]);
		if ((NULL != source_file) && !(TM_SOURCE_FILE(source_file)->inactive) &&
		  (NULL != source_file->tags_array) && (source_file->tags_array->len > 0))
		{
			for (j = 0; j < source_file->tags_array->len; ++j)
			{
				g_ptr_array_add(project->work_object.tags_array,
					  source_file->tags_array->pdata[j]);
			}
		}
	}
	tm_tags_sort(project->work_object.tags_array, NULL, FALSE);
}

gboolean tm_project_update(TMWorkObject *work_object, gboolean force
  , gboolean recurse, gboolean update_parent)
{
	TMProject *project;
	guint i;
	gboolean update_tags = force;

	if (!work_object || !IS_TM_PROJECT(work_object))
	{
		g_warning("Non project pointer passed to project update");
		return FALSE;
	}

#ifdef DEBUG
	g_message("Updating project: %s", work_object->file_name);
#endif

	project = TM_PROJECT(work_object);
	if ((NULL != project->file_list) && (project->file_list->len > 0))
	{
		if (recurse)
		{
			for (i=0; i < project->file_list->len; ++i)
			{
				if (TRUE == tm_source_file_update(TM_WORK_OBJECT(
					  project->file_list->pdata[i]), FALSE, FALSE, FALSE))
					update_tags = TRUE;
			}
		}
		if (update_tags)
			tm_project_recreate_tags_array(project);
	}
	work_object->analyze_time = time(NULL);
	if ((work_object->parent) && (update_parent) && (update_tags))
		tm_workspace_update(work_object->parent, TRUE, FALSE, FALSE);
	return update_tags;
}

gboolean tm_project_open(TMProject *project, gboolean force)
{
	FILE *fp;
	TMSourceFile *source_file = NULL;
	TMTag *tag;

	if (!project || !IS_TM_PROJECT(TM_WORK_OBJECT(project)))
		return FALSE;
#ifdef DEBUG
	g_message("Opening project %s", project->work_object.file_name);
#endif
	if (NULL == (fp = fopen(project->work_object.file_name, "r")))
		return FALSE;
	while (NULL != (tag = tm_tag_new_from_file(source_file, fp)))
	{
		if (tm_tag_file_t == tag->type)
		{
			if (!(source_file = TM_SOURCE_FILE(
			  tm_source_file_new(tag->name, FALSE))))
			{
#ifdef DEBUG
				g_warning("Unable to create source file %s", tag->name);
#endif
				if (!force)
				{
					tm_tag_free(tag);
					fclose(fp);
					return FALSE;
				}
				else
					source_file = NULL;
			}
			else
			{
				source_file->work_object.analyze_time = tag->atts.file.timestamp;
				source_file->lang = tag->atts.file.lang;
				source_file->inactive = tag->atts.file.inactive;
				if (!project->file_list)
					project->file_list = g_ptr_array_new();
				g_ptr_array_add(project->file_list, source_file);
			}
			tm_tag_free(tag);
		}
		else
		{
			if ((NULL == source_file) || (source_file->inactive)) /* Dangling tag */
			{
#ifdef DEBUG
				g_warning("Dangling tag %s", tag->name);
#endif
				tm_tag_free(tag);
				if (!force)
				{
					fclose(fp);
					return FALSE;
				}
			}
			else
			{
				if (NULL == source_file->work_object.tags_array)
					source_file->work_object.tags_array = g_ptr_array_new();
				g_ptr_array_add(source_file->work_object.tags_array, tag);
			}
		}
	}
	fclose(fp);
	tm_project_recreate_tags_array(project);
	return TRUE;
}

gboolean tm_project_save(TMProject *project)
{
	guint i;
	FILE *fp;

	if (!project)
		return FALSE;
	if (NULL == (fp = fopen(project->work_object.file_name, "w")))
	{
		g_warning("Unable to save project %s", project->work_object.file_name);
		return FALSE;
	}
	if (project->file_list)
	{
		for (i=0; i < project->file_list->len; ++i)
		{
			if (FALSE == tm_source_file_write(TM_WORK_OBJECT(project->file_list->pdata[i])
				, fp, tm_tag_attr_max_t))
			{
				fclose(fp);
				return FALSE;
			}
		}
	}
	fclose(fp);
	return TRUE;
}

static void tm_project_add_file_recursive(TMFileEntry *entry
  , gpointer user_data, guint __unused__ level)
{
	TMProject *project;
	if (!user_data || !entry || (tm_file_dir_t == entry->type))
		return;
	project = TM_PROJECT(user_data);
	tm_project_add_file(project, entry->path, FALSE);
}

gboolean tm_project_autoscan(TMProject *project)
{
	static char *makefile[] = { "Makefile.am", "Makefile.in", "Makefile" };
	char buf[PATH_MAX];
	gboolean status = FALSE;
	TMFileEntry *root_dir;
	guint i;
	struct stat s;

	if (!project || !IS_TM_PROJECT(TM_WORK_OBJECT(project))
	  || (!project->dir))
		return FALSE;
	for (i = 0; i < sizeof(makefile)/sizeof(char *); ++i)
	{
		g_snprintf(buf, PATH_MAX, "%s/%s", project->dir, makefile[i]);
		if (0 == stat(buf, &s) && S_ISREG(s.st_mode))
		{
			status = TRUE;
			break;
		}
	}
	if (!status)
	{
		g_warning("%s is not a top level project directory", project->dir);
		return FALSE;
	}
	if (!(root_dir = tm_file_entry_new(project->dir, NULL, TRUE
		, project->sources, project->ignore, TRUE)))
	{
		g_warning("Unable to create file entry");
		return FALSE;
	}
	tm_file_entry_foreach(root_dir, tm_project_add_file_recursive
	  , project, 0, FALSE);
	tm_file_entry_free(root_dir);
	tm_project_update(TM_WORK_OBJECT(project), TRUE, FALSE, TRUE);
	return TRUE;
}

gboolean tm_project_is_source_file(TMProject *project, const char *file_name)
{
	const char **pr_extn;

	if (!(project && IS_TM_PROJECT(TM_WORK_OBJECT(project))
	  && file_name && project->sources))
		return FALSE;
	for (pr_extn = project->sources; pr_extn && *pr_extn; ++ pr_extn)
	{
		if (0 == fnmatch(*pr_extn, file_name, 0))
			return TRUE;
	}
	return FALSE;
}
