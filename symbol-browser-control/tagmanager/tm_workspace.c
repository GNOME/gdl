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
#include <sys/types.h>
#include <string.h>

#include "tm_tag.h"
#include "tm_workspace.h"

static TMWorkspace *theWorkspace = NULL;
guint workspace_class_id = 0;

static gboolean tm_create_workspace(void)
{
	char *file_name = g_strdup_printf("%s/%d", P_tmpdir, getpid());
#ifdef DEBUG
	g_message("Workspace created: %s", file_name);
#endif

	workspace_class_id = tm_work_object_register(tm_workspace_free, tm_workspace_update
		  , tm_workspace_find_object);
	theWorkspace = g_new(TMWorkspace, 1);
	if (FALSE == tm_work_object_init(TM_WORK_OBJECT(theWorkspace),
		  workspace_class_id, file_name, TRUE))
	{
		g_free(file_name);
		g_free(theWorkspace);
		theWorkspace = NULL;
		g_warning("Failed to initialize workspace");
		return FALSE;
	}

	g_free(file_name);
	theWorkspace->global_tags = NULL;
	theWorkspace->work_objects = NULL;
	return TRUE;
}

void tm_workspace_free(gpointer workspace)
{
	guint i;

	if (workspace != theWorkspace)
		return;

#ifdef DEBUG
	g_message("Workspace destroyed");
#endif

	if (NULL != theWorkspace)
	{
		if (NULL != theWorkspace->work_objects)
		{
			for (i=0; i < theWorkspace->work_objects->len; ++i)
				tm_work_object_free(theWorkspace->work_objects->pdata[i]);
			g_ptr_array_free(theWorkspace->work_objects, TRUE);
		}
		if (NULL != theWorkspace->global_tags)
		{
			for (i=0; i < theWorkspace->global_tags->len; ++i)
				tm_tag_free(theWorkspace->global_tags->pdata[i]);
			g_ptr_array_free(theWorkspace->global_tags, TRUE);
		}
		unlink(theWorkspace->work_object.file_name);
		tm_work_object_destroy(TM_WORK_OBJECT(theWorkspace));
		g_free(theWorkspace);
		theWorkspace = NULL;
	}
}

const TMWorkspace *tm_get_workspace(void)
{
	if (NULL == theWorkspace)
		tm_create_workspace();
	return theWorkspace;
}

gboolean tm_workspace_add_object(TMWorkObject *work_object)
{
	if (NULL == theWorkspace)
		tm_create_workspace();
	if (NULL == theWorkspace->work_objects)
		theWorkspace->work_objects = g_ptr_array_new();
	g_ptr_array_add(theWorkspace->work_objects, work_object);
	work_object->parent = TM_WORK_OBJECT(theWorkspace);
	return TRUE;
}

gboolean tm_workspace_remove_object(TMWorkObject *w, gboolean free)
{
	guint i;
	if ((NULL == theWorkspace) || (NULL == theWorkspace->work_objects)
		  || (NULL == w))
		return FALSE;
	for (i=0; i < theWorkspace->work_objects->len; ++i)
	{
		if (theWorkspace->work_objects->pdata[i] == w)
		{
			if (free)
				tm_work_object_free(w);
			g_ptr_array_remove_index_fast(theWorkspace->work_objects, i);
			tm_workspace_update(TM_WORK_OBJECT(theWorkspace), TRUE, FALSE, FALSE);
			return TRUE;
		}
	}
	return FALSE;
}

gboolean tm_workspace_load_global_tags(const char *tags_file)
{
	FILE *fp;
	TMTag *tag;

	if (NULL == (fp = fopen(tags_file, "r")))
		return FALSE;
	if (NULL == theWorkspace)
		tm_create_workspace();
	if (NULL == theWorkspace->global_tags)
		theWorkspace->global_tags = g_ptr_array_new();
	while (NULL != (tag = tm_tag_new_from_file(NULL, fp)))
		g_ptr_array_add(theWorkspace->global_tags, tag);
	fclose(fp);
	return TRUE;
}

gboolean tm_workspace_create_global_tags(const char *pre_process, const char *includes
  , const char *tags_file)
{
	char *command;
	guint i;
	FILE *fp;
	TMWorkObject *source_file;
	GPtrArray *tags_array;
	char *temp_file = g_strdup_printf("%s/%d_%ld_1.cpp", P_tmpdir, getpid(), time(NULL));
	char *temp_file2 = g_strdup_printf("%s/%d_%ld_2.cpp", P_tmpdir, getpid(), time(NULL));
	TMTagAttrType sort_attrs[] = { tm_tag_attr_name_t, tm_tag_attr_scope_t
		, tm_tag_attr_type_t, 0};
	if (NULL == (fp = fopen(temp_file, "w")))
		return FALSE;
	fclose(fp);
	command = g_strdup_printf("\
	  for file in %s; \
	  do \
	  	echo \"#include \\\"$file\\\"\" >>%s; \
	  done", includes, temp_file);
#ifdef DEBUG
	g_warning("Executing: %s", command);
#endif
	if (0 != system(command))
	{
		g_free(command);
		g_free(temp_file);
		return FALSE;
	}
	g_free(command);
	command = g_strdup_printf("%s %s >%s", pre_process, temp_file, temp_file2);
#ifdef DEBUG
	g_warning("Executing: %s", command);
#endif
	system(command);
	g_free(command);
	unlink(temp_file);
	g_free(temp_file);
	source_file = tm_source_file_new(temp_file2, TRUE);
	if (NULL == source_file)
	{
		unlink(temp_file2);
		return FALSE;
	}
	unlink(temp_file2);
	g_free(temp_file2);
	if ((NULL == source_file->tags_array) || (0 == source_file->tags_array->len))
	{
		tm_source_file_free(source_file);
		return FALSE;
	}
	tags_array = tm_tags_extract(source_file->tags_array, tm_tag_class_t |
	  tm_tag_typedef_t | tm_tag_prototype_t | tm_tag_enum_t | tm_tag_macro_with_arg_t);
	if ((NULL == tags_array) || (0 == tags_array->len))
	{
		g_ptr_array_free(tags_array, TRUE);
		tm_source_file_free(source_file);
		return FALSE;
	}
	if (FALSE == tm_tags_sort(tags_array, sort_attrs, TRUE))
	{
		tm_source_file_free(source_file);
		return FALSE;
	}
	if (NULL == (fp = fopen(tags_file, "w")))
	{
		tm_source_file_free(source_file);
		return FALSE;
	}
	for (i=0; i < tags_array->len; ++i)
	{
		tm_tag_write(TM_TAG(tags_array->pdata[i]), fp, tm_tag_attr_type_t
		  | tm_tag_attr_scope_t | tm_tag_attr_arglist_t | tm_tag_attr_vartype_t);
	}
	fclose(fp);
	tm_source_file_free(source_file);
	g_ptr_array_free(tags_array, TRUE);
	return TRUE;
}

TMWorkObject *tm_workspace_find_object(TMWorkObject *work_object, const char *file_name
  , gboolean name_only)
{
	TMWorkObject *w = NULL;
	guint i;

	if (work_object != TM_WORK_OBJECT(theWorkspace))
		return NULL;
	if ((NULL == theWorkspace) || (NULL == theWorkspace->work_objects)
		|| (0 == theWorkspace->work_objects->len))
		return NULL;
	for (i = 0; i < theWorkspace->work_objects->len; ++i)
	{
		if (NULL != (w = tm_work_object_find(TM_WORK_OBJECT(theWorkspace->work_objects->pdata[i])
			  , file_name, name_only)))
			return w;
	}
	return NULL;
}

void tm_workspace_recreate_tags_array(void)
{
	guint i, j;
	TMWorkObject *w;
	TMTagAttrType sort_attrs[] = { tm_tag_attr_name_t, tm_tag_attr_file_t
		, tm_tag_attr_scope_t, tm_tag_attr_type_t, 0};

#ifdef DEBUG
	g_message("Recreating workspace tags array");
#endif

	if ((NULL == theWorkspace) || (NULL == theWorkspace->work_objects))
		return;
	if (NULL != theWorkspace->work_object.tags_array)
		g_ptr_array_set_size(theWorkspace->work_object.tags_array, 0);
	else
		theWorkspace->work_object.tags_array = g_ptr_array_new();

#ifdef DEBUG
	g_message("Total %d objects", theWorkspace->work_objects->len);
#endif
	for (i=0; i < theWorkspace->work_objects->len; ++i)
	{
		w = TM_WORK_OBJECT(theWorkspace->work_objects->pdata[i]);
#ifdef DEBUG
		g_message("Adding tags of %s", w->file_name);
#endif
		if ((NULL != w) && (NULL != w->tags_array) && (w->tags_array->len > 0))
		{
			for (j = 0; j < w->tags_array->len; ++j)
			{
				g_ptr_array_add(theWorkspace->work_object.tags_array,
					  w->tags_array->pdata[j]);
			}
		}
	}
#ifdef DEBUG
	g_message("Total: %d tags", theWorkspace->work_object.tags_array->len);
#endif
	tm_tags_sort(theWorkspace->work_object.tags_array, sort_attrs, TRUE);
}

gboolean tm_workspace_update(TMWorkObject *workspace, gboolean force
  , gboolean recurse, gboolean __unused__ update_parent)
{
	guint i;
	gboolean update_tags = force;
	
#ifdef DEBUG
	g_message("Updating workspace");
#endif

	if (workspace != TM_WORK_OBJECT(theWorkspace))
		return FALSE;
	if (NULL == theWorkspace)
		return TRUE;
	if ((recurse) && (theWorkspace->work_objects))
	{
		for (i=0; i < theWorkspace->work_objects->len; ++i)
		{
			if (TRUE == tm_work_object_update(TM_WORK_OBJECT(
				  theWorkspace->work_objects->pdata[i]), FALSE, TRUE, FALSE))
				update_tags = TRUE;
		}
	}
	if (update_tags)
		tm_workspace_recreate_tags_array();
	workspace->analyze_time = time(NULL);
	return update_tags;
}

const GPtrArray *tm_workspace_find(const char *name, int type, TMTagAttrType *attrs
 , gboolean partial)
{
	static GPtrArray *tags = NULL;
	TMTag **matches[2], **match;
	int i, len;

	if ((!theWorkspace) || (!name))
		return NULL;
	len = strlen(name);
	if (!len)
		return NULL;
	if (tags)
		g_ptr_array_set_size(tags, 0);
	else
		tags = g_ptr_array_new();

	matches[0] = tm_tags_find(theWorkspace->work_object.tags_array, name, partial);
	matches[1] = tm_tags_find(theWorkspace->global_tags, name, partial);
	for (i = 0; i < 2; ++i)
	{
		match = matches[i];
		if ((match) && (*match))
		{
			while (TRUE)
			{
				if (type & (*match)->type)
					g_ptr_array_add(tags, *match);
				++ match;
				if (NULL == *match)
					break;
				if (partial)
				{
					if (0 != strncmp((*match)->name, name, len))
						break;
				}
				else
				{
					if (0 != strcmp((*match)->name, name))
						break;
				}
			}
		}
	}
	tm_tags_sort(tags, attrs, TRUE);
	return tags;
}
