#include <string.h>
#include <sys/stat.h>

#include "tm_tag.h"
#include "tm_workspace.h"
#include "tm_project.h"

#define TM_FILE_NAME "tm.tags"

guint project_class_id = 0;

gboolean tm_project_init(TMProject *project, const char *file_name,
  const char *project_name)
{
	struct stat s;
	gboolean directory = FALSE;
	gboolean exists = FALSE;
	char *real_file_name = (char *) file_name;

	g_return_val_if_fail((project && file_name), FALSE);
#ifdef TM_DEBUG
	g_message("Initializing project %s", file_name);
#endif
	if (0 == project_class_id)
		project_class_id = tm_work_object_register(tm_project_free, tm_project_update
		  , tm_project_find_file);

	if (0 == stat(file_name, &s))
	{
		exists = TRUE;
		if (S_ISDIR(s.st_mode))
		{
			directory = TRUE;
			real_file_name = g_strdup_printf("%s/%s", file_name, TM_FILE_NAME);
			if ((0 == stat(real_file_name, &s)) && S_ISREG(s.st_mode))
				exists = TRUE;
			else
				exists = FALSE;
		}
	}
	if (FALSE == tm_work_object_init(&(project->work_object),
		  project_class_id, real_file_name, TRUE))
	{
		if (file_name != real_file_name)
			g_free(real_file_name);
		return FALSE;
	}
	if (NULL != project_name)
		project->project_name = g_strdup(project_name);
	else
		project->project_name = NULL;
	project->file_list = NULL;
	project->symbol_tree = NULL;
	tm_workspace_add_object(TM_WORK_OBJECT(project));
	if (!exists)
	{
		if (directory)
			tm_project_autoscan(project, file_name);
	}
	else
		tm_project_open(project, FALSE);
	if (file_name != real_file_name)
		g_free(real_file_name);
	return TRUE;
}

TMWorkObject *tm_project_new(const char *file_name, const char *project_name)
{
	TMProject *project = g_new(TMProject, 1);
	if (FALSE == tm_project_init(project, file_name, project_name))
	{
		g_free(project);
		return NULL;
	}
	return (TMWorkObject *) project;
}

void tm_project_destroy(TMProject *project)
{
	g_assert(project);
#ifdef TM_DEBUG
	g_message("Destroying project: %s", project->work_object.file_name);
#endif

	tm_project_save(project);
	if (NULL != project->file_list)
	{
		int i;
		for (i = 0; i < project->file_list->len; ++i)
			tm_source_file_free(TM_SOURCE_FILE(project->file_list->pdata[i]));
		g_ptr_array_free(project->file_list, TRUE);
	}
	tm_workspace_remove_object(TM_WORK_OBJECT(project), FALSE);
	g_free(project->project_name);
	tm_work_object_destroy(&(project->work_object));
}

void tm_project_free(gpointer project)
{
	if (NULL != project)
	{
		tm_project_destroy(project);
		g_free(project);
	}
}

gboolean tm_project_add_file(TMProject *project, const char *file_name, gboolean update)
{
	TMWorkObject *source_file;
	g_return_val_if_fail((project && file_name), FALSE);
	if (NULL != tm_project_find_file(TM_WORK_OBJECT(project), file_name))
		return TRUE;
	if (NULL == (source_file = tm_source_file_new(file_name, update)))
		return FALSE;
	source_file->parent = TM_WORK_OBJECT(project);
	if (NULL == project->file_list)
		project->file_list = g_ptr_array_new();
	g_ptr_array_add(project->file_list, source_file);
	if (update)
		tm_project_update(TM_WORK_OBJECT(project), FALSE, TRUE, TRUE);
	return TRUE;
}

TMWorkObject *tm_project_find_file(TMWorkObject *work_object, const char *file_name)
{
	TMProject *project;
	if ((NULL == work_object) || (NULL == file_name))
		return NULL;
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
		int i;
		for (i=0; i < project->file_list->len; ++i)
		{
			if (0 == strcmp(TM_WORK_OBJECT(project->file_list->pdata[i])->file_name
				  , file_name))
				return TM_WORK_OBJECT(project->file_list->pdata[i]);
		}
	}
	return NULL;
}

gboolean tm_project_remove_object(TMProject *project, TMWorkObject *w)
{
	int i;

	g_return_val_if_fail((project && w), FALSE);
	if (!project->file_list)
		return FALSE;
	for (i=0; i < project->file_list->len; ++i)
	{
		if (w == project->file_list->pdata[i])
		{
			tm_work_object_free(w);
			g_ptr_array_remove_index_fast(project->file_list, i);
			tm_project_update(TM_WORK_OBJECT(project), TRUE, FALSE, TRUE);
			return TRUE;
		}
	}
	return FALSE;
}

void tm_project_recreate_tags_array(TMProject *project)
{
	int i, j;
	TMWorkObject *source_file;
	TMTagAttrType sort_attrs[] = { tm_tag_attr_name_t, tm_tag_attr_scope_t,
		tm_tag_attr_type_t, 0};

	g_assert(project);
#ifdef TM_DEBUG
	g_message("Recreating tags for project: %s", project->work_object.file_name);
#endif

	if (NULL != project->work_object.tags_array)
		g_ptr_array_set_size(project->work_object.tags_array, 0);
	else
		project->work_object.tags_array = g_ptr_array_new();

	for (i=0; i < project->file_list->len; ++i)
	{
		source_file = TM_WORK_OBJECT(project->file_list->pdata[i]);
		if ((NULL != source_file) && (NULL != source_file->tags_array)
			  && (source_file->tags_array->len > 0))
		{
			for (j = 0; j < source_file->tags_array->len; ++j)
			{
				g_ptr_array_add(project->work_object.tags_array,
					  source_file->tags_array->pdata[j]);
			}
		}
	}
	tm_tags_sort(project->work_object.tags_array, sort_attrs, TRUE);
	project->symbol_tree = tm_symbol_tree_update(project->symbol_tree
	  , project->work_object.tags_array);
}

gboolean tm_project_update(TMWorkObject *work_object, gboolean force
  , gboolean recurse, gboolean update_parent)
{
	TMProject *project;
	int i;
	gboolean update_tags = force;

	g_return_val_if_fail(IS_TM_PROJECT(work_object), FALSE);

#ifdef TM_DEBUG
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

	g_assert(project);
	if (NULL == (fp = fopen(project->work_object.file_name, "r")))
		return FALSE;
	while (NULL != (tag = tm_tag_new_from_file(source_file, fp)))
	{
		if (tm_tag_file_t == tag->type)
		{
			if (FALSE == tm_project_add_file(project, tag->name, FALSE))
			{
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
				source_file = TM_SOURCE_FILE(
				  project->file_list->pdata[project->file_list->len - 1]);
				source_file->work_object.analyze_time = tag->atts.file.timestamp;
			}
			tm_tag_free(tag);
		}
		else
		{
			if (NULL == source_file) /* Dangling tag */
			{
				g_warning("Dangling tag %s", tag->name);
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
	return tm_project_update(TM_WORK_OBJECT(project), TRUE, TRUE, TRUE);
}

gboolean tm_project_save(TMProject *project)
{
	int i;
	FILE *fp;

	g_assert(project);
	tm_project_update(TM_WORK_OBJECT(project), FALSE, TRUE, TRUE);
	if (NULL == (fp = fopen(project->work_object.file_name, "w")))
		return FALSE;
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

gboolean tm_project_autoscan(TMProject *project, const char *dir_name)
{
	static char *extn[] = {"*.h", "*.c", "*.cpp", "*.cc", "*.cxx", "*.C" };
	static char *makefile[] = { "Makefile.am", "Makefile.in", "Makefile" };
	struct stat s;
	int i;
	int len;
	char buf[BUFSIZ];
	FILE *p;
	gboolean status = FALSE;

	g_assert(project);
	g_assert(dir_name);
	for (i = 0; i < sizeof(makefile)/sizeof(char *); ++i)
	{
		g_snprintf(buf, BUFSIZ, "%s/%s", dir_name, makefile[i]);
		if (0 == stat(buf, &s) && S_ISREG(s.st_mode))
		{
			status = TRUE;
			break;
		}
	}
	if (!status)
	{
		g_warning("%s if not a top level project directory", dir_name);
		return FALSE;
	}
	for (i=0; i < sizeof(extn)/sizeof(char *); ++i)
	{
		g_snprintf(buf, BUFSIZ, "find %s -name \"%s\" -type f -print", dir_name, extn[i]);
		if (NULL != (p = popen(buf, "r")))
		{
			while (NULL != fgets(buf, BUFSIZ, p))
			{
				len = strlen(buf);
				if ('\n' == buf[len - 1])
					buf[len-1] = '\0';
				tm_project_add_file(project, buf, FALSE);
			}
		}
	}
	tm_project_update(TM_WORK_OBJECT(project), TRUE, TRUE, TRUE);
	return TRUE;
}
