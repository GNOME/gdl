#include <stdlib.h>
#include <string.h>

#include "general.h"
#include "entry.h"
#include "parse.h"
#include "read.h"
#define LIBCTAGS_DEFINED
#include "tm_tag.h"

static GMemChunk *tag_mem_chunk = NULL;

#define TAG_NEW(T) {\
	if (!tag_mem_chunk) \
		tag_mem_chunk = g_mem_chunk_new("TMTag memChunk", sizeof(TMTag), 1024 \
		  , G_ALLOC_AND_FREE); \
	(T) = g_chunk_new0(TMTag, tag_mem_chunk);}

#define TAG_FREE(T) g_mem_chunk_free(tag_mem_chunk, (T))

enum
{
	TA_NAME = 200,
	TA_LINE,
	TA_LOCAL,
	TA_POS,
	TA_TYPE,
	TA_ARGLIST,
	TA_SCOPE,
	TA_VARTYPE,
	TA_INHERITS,
	TA_TIME
};

static guint *s_sort_attrs = NULL;
static gboolean s_partial = FALSE;

static TMTagType tag_types[] = {
	tm_tag_undef_t, tm_tag_class_t, tm_tag_enum_t, tm_tag_enumerator_t,
	tm_tag_field_t,	tm_tag_function_t, tm_tag_interface_t, tm_tag_member_t,
	tm_tag_method_t, tm_tag_namespace_t, tm_tag_package_t, tm_tag_prototype_t,
	tm_tag_struct_t, tm_tag_typedef_t, tm_tag_union_t, tm_tag_variable_t,
	tm_tag_externvar_t,	tm_tag_macro_t,	tm_tag_macro_with_arg_t,
	tm_tag_file_t, tm_tag_max_t
};

gboolean tm_tag_init(TMTag *tag, TMSourceFile *file, const tagEntryInfo *tag_entry)
{
	if (NULL == tag_entry)
	{
		/* This is a file tag */
		if (NULL == file)
			return FALSE;
		else
		{
			tag->name = g_strdup(file->work_object.file_name);
			tag->type = tm_tag_file_t;
			tag->atts.file.timestamp = file->work_object.analyze_time;
			tag->atts.file.lang = file->lang;
			return TRUE;
		}
	}
	else
	{
		/* This is a normal tag entry */
		if ((tag_entry->type <= TAG_UNDEFINED) || (tag_entry->type >= TAG_COUNT))
			return FALSE;
		if (NULL == tag_entry->name)
			return FALSE;
		tag->name = g_strdup(tag_entry->name);
		tag->type = tag_types[tag_entry->type];
		tag->atts.entry.local = tag_entry->isFileScope;
		tag->atts.entry.line = tag_entry->lineNumber;
		tag->atts.entry.pos = tag_entry->seekPosition;
		if (NULL != tag_entry->extensionFields.arglist)
			tag->atts.entry.arglist = g_strdup(tag_entry->extensionFields.arglist);
		if ((NULL != tag_entry->extensionFields.scope[1]) &&
			(isalpha(tag_entry->extensionFields.scope[1][0]) || 
			 tag_entry->extensionFields.scope[1][0] == '_'))
			tag->atts.entry.scope = g_strdup(tag_entry->extensionFields.scope[1]);
		if (tag_entry->extensionFields.inheritance != NULL)
			tag->atts.entry.inheritance = g_strdup(tag_entry->extensionFields.inheritance);
		if (tag_entry->extensionFields.varType != NULL)
			tag->atts.entry.var_type = g_strdup(tag_entry->extensionFields.varType);
		if ((tm_tag_macro_t == tag->type) && (NULL != tag->atts.entry.arglist))
			tag->type = tm_tag_macro_with_arg_t;
		return TRUE;
	}
}

TMTag *tm_tag_new(TMSourceFile *file, const tagEntryInfo *tag_entry)
{
	TMTag *tag;

	TAG_NEW(tag);
	if (FALSE == tm_tag_init(tag, file, tag_entry))
	{
		TAG_FREE(tag);
		return NULL;
	}
	return tag;
}

gboolean tm_tag_init_from_file(TMTag *tag, TMSourceFile *file, FILE *fp)
{
	guchar buf[BUFSIZ];
	guchar *start, *end;
	gboolean status;
	guchar changed_char = TA_NAME;

	if ((NULL == fgets(buf, BUFSIZ, fp)) || ('\0' == *buf))
		return FALSE;
	for (start = end = buf, status = TRUE; (TRUE == status); start = end, ++ end)
	{
		while ((*end < TA_NAME) && (*end != '\0') && (*end != '\n'))
			++ end;
		if (('\0' == *end) || ('\n' == *end))
			status = FALSE;
		changed_char = *end;
		*end = '\0';
		if (NULL == tag->name)
		{
			if (!isprint(*start))
				return FALSE;
			else
				tag->name = g_strdup(start);
		}
		else
		{
			switch (*start)
			{
				case TA_LINE:
					tag->atts.entry.line = atol(start + 1);
					break;
				case TA_LOCAL:
					tag->atts.entry.local = atoi(start + 1);
					break;
				case TA_POS:
					tag->atts.entry.pos = atol(start + 1);
					break;
				case TA_TYPE:
					tag->type = (TMTagType) atoi(start + 1);
					break;
				case TA_ARGLIST:
					tag->atts.entry.arglist = g_strdup(start + 1);
					break;
				case TA_SCOPE:
					tag->atts.entry.scope = g_strdup(start + 1);
					break;
				case TA_VARTYPE:
					tag->atts.entry.var_type = g_strdup(start + 1);
					break;
				case TA_INHERITS:
					tag->atts.entry.inheritance = g_strdup(start + 1);
					break;
				case TA_TIME:
					if (tm_tag_file_t != tag->type)
					{
						g_warning("Got time attribute for non-file tag %s", tag->name);
						return FALSE;
					}
					else
						tag->atts.file.timestamp = atol(start + 1);
					break;
				default:
					g_warning("Unknown attribute %s", start + 1);
					break;
			}
		}
		*end = changed_char;
	}
	if (NULL == tag->name)
		return FALSE;
	if (tm_tag_file_t != tag->type)
		tag->atts.entry.file = file;
	return TRUE;
}

TMTag *tm_tag_new_from_file(TMSourceFile *file, FILE *fp)
{
	TMTag *tag;

	TAG_NEW(tag);
	if (FALSE == tm_tag_init_from_file(tag, file, fp))
	{
		TAG_FREE(tag);
		return NULL;
	}
	return tag;
}

gboolean tm_tag_write(TMTag *tag, FILE *fp, guint attrs)
{
	fprintf(fp, "%s", tag->name);
	if (attrs & tm_tag_attr_type_t)
		fprintf(fp, "%c%d", TA_TYPE, tag->type);
	if (tag->type == tm_tag_file_t)
	{
		if (attrs & tm_tag_attr_time_t)
			fprintf(fp, "%c%ld", TA_TIME, tag->atts.file.timestamp);
	}
	else
	{
		if ((attrs & tm_tag_attr_arglist_t) && (NULL != tag->atts.entry.arglist))
			fprintf(fp, "%c%s", TA_ARGLIST, tag->atts.entry.arglist);
		if (attrs & tm_tag_attr_line_t)
			fprintf(fp, "%c%ld", TA_LINE, tag->atts.entry.line);
		if (attrs & tm_tag_attr_local_t)
			fprintf(fp, "%c%d", TA_LOCAL, tag->atts.entry.local);
		if (attrs & tm_tag_attr_pos_t)
			fprintf(fp, "%c%ld", TA_POS, tag->atts.entry.pos);
		if ((attrs & tm_tag_attr_scope_t) && (NULL != tag->atts.entry.scope))
			fprintf(fp, "%c%s", TA_SCOPE, tag->atts.entry.scope);
		if ((attrs & tm_tag_attr_vartype_t) && (NULL != tag->atts.entry.var_type))
			fprintf(fp, "%c%s", TA_VARTYPE, tag->atts.entry.var_type);
		if ((attrs & tm_tag_attr_inheritance_t) && (NULL != tag->atts.entry.inheritance))
			fprintf(fp, "%c%s", TA_INHERITS, tag->atts.entry.inheritance);
	}
	if (fprintf(fp, "\n"))
		return TRUE;
	else
		return FALSE;
}

void tm_tag_destroy(TMTag *tag)
{
	g_free(tag->name);
	if (tm_tag_file_t != tag->type)
	{
		g_free(tag->atts.entry.arglist);
		g_free(tag->atts.entry.scope);
		g_free(tag->atts.entry.inheritance);
		g_free(tag->atts.entry.var_type);
	}
}

void tm_tag_free(gpointer tag)
{
	if (NULL != tag)
	{
		tm_tag_destroy((TMTag *) tag);
		TAG_FREE(tag);
	}
}

int tm_tag_compare(const void *ptr1, const void *ptr2)
{
	int *sort_attr;
	int returnval = 0;
	TMTag *t1 = *((TMTag **) ptr1);
	TMTag *t2 = *((TMTag **) ptr2);

	if ((NULL == t1) || (NULL == t2))
	{
		g_warning("Found NULL tag");
		return t2 - t1;
	}
	if (NULL == s_sort_attrs)
	{
		if (s_partial)
			return strncmp(NVL(t1->name, ""), NVL(t2->name, ""), strlen(NVL(t1->name, "")));
		else
			return strcmp(NVL(t1->name, ""), NVL(t2->name, ""));
	}

	for (sort_attr = s_sort_attrs; *sort_attr != tm_tag_attr_none_t; ++ sort_attr)
	{
		switch (*sort_attr)
		{
			case tm_tag_attr_name_t:
				if (s_partial)
					returnval = strncmp(NVL(t1->name, ""), NVL(t2->name, ""), strlen(NVL(t1->name, "")));
				else
					returnval = strcmp(NVL(t1->name, ""), NVL(t2->name, ""));
				if (0 != returnval)
					return returnval;
				break;
			case tm_tag_attr_type_t:
				if (0 != (returnval = (t1->type - t2->type)))
					return returnval;
				break;
			case tm_tag_attr_file_t:
				if (0 != (returnval = (t1->atts.entry.file - t2->atts.entry.file)))
					return returnval;
				break;
			case tm_tag_attr_scope_t:
				if (0 != (returnval = strcmp(NVL(t1->atts.entry.scope, ""), NVL(t2->atts.entry.scope, ""))))
					return returnval;
				break;
			case tm_tag_attr_vartype_t:
				if (0 != (returnval = strcmp(NVL(t1->atts.entry.var_type, ""), NVL(t2->atts.entry.var_type, ""))))
					return returnval;
				break;
			case tm_tag_attr_line_t:
				if (0 != (returnval = (t1->atts.entry.line - t2->atts.entry.line)))
					return returnval;
				break;
		}
	}
	return returnval;
}

gboolean tm_tags_prune(GPtrArray *tags_array)
{
	int i, count;
	for (i=0, count = 0; i < tags_array->len; ++i)
	{
		if (NULL != tags_array->pdata[i])
			tags_array->pdata[count++] = tags_array->pdata[i];
	}
	tags_array->len = count;
	return TRUE;
}

gboolean tm_tags_dedup(GPtrArray *tags_array, TMTagAttrType *sort_attributes)
{
	int i;

	if ((!tags_array) || (!tags_array->len))
		return TRUE;
	s_sort_attrs = sort_attributes;
	s_partial = FALSE;
	for (i = 1; i < tags_array->len; ++i)
	{
		if (0 == tm_tag_compare(&(tags_array->pdata[i - 1]), &(tags_array->pdata[i])))
			tags_array->pdata[i-1] = NULL;
	}
	tm_tags_prune(tags_array);
	return TRUE;
}

gboolean tm_tags_custom_dedup(GPtrArray *tags_array, TMTagCompareFunc compare_func)
{
	int i;

	if ((!tags_array) || (!tags_array->len))
		return TRUE;
	for (i = 1; i < tags_array->len; ++i)
	{
		if (0 == compare_func(&(tags_array->pdata[i - 1]), &(tags_array->pdata[i])))
			tags_array->pdata[i-1] = NULL;
	}
	tm_tags_prune(tags_array);
	return TRUE;
}

gboolean tm_tags_sort(GPtrArray *tags_array, TMTagAttrType *sort_attributes, gboolean dedup)
{
	if ((!tags_array) || (!tags_array->len))
		return TRUE;
	s_sort_attrs = sort_attributes;
	s_partial = FALSE;
	qsort(tags_array->pdata, tags_array->len, sizeof(gpointer), tm_tag_compare);
	s_sort_attrs = NULL;
	if (dedup)
		tm_tags_dedup(tags_array, sort_attributes);
	return TRUE;
}

gboolean tm_tags_custom_sort(GPtrArray *tags_array, TMTagCompareFunc compare_func, gboolean dedup)
{
	if ((!tags_array) || (!tags_array->len))
		return TRUE;
	qsort(tags_array->pdata, tags_array->len, sizeof(gpointer), compare_func);
	if (dedup)
		tm_tags_custom_dedup(tags_array, compare_func);
	return TRUE;
}

GPtrArray *tm_tags_extract(GPtrArray *tags_array, guint tag_types)
{
	GPtrArray *new_tags;
	int i;
	if (NULL == tags_array)
		return NULL;
	new_tags = g_ptr_array_new();
	for (i=0; i < tags_array->len; ++i)
	{
		if (NULL != tags_array->pdata[i])
		{
			if (tag_types & (((TMTag *) tags_array->pdata[i])->type))
				g_ptr_array_add(new_tags, tags_array->pdata[i]);
		}
	}
	return new_tags;
}

void tm_tags_array_free(GPtrArray *tags_array, gboolean free_all)
{
	if (NULL != tags_array)
	{
		int i;
		for (i = 0; i < tags_array->len; ++i)
			tm_tag_free(tags_array->pdata[i]);
		if (free_all)
			g_ptr_array_free(tags_array, TRUE);
		else
			g_ptr_array_set_size(tags_array, 0);
	}
}

TMTag **tm_tags_find(GPtrArray *sorted_tags_array, const char *name, gboolean partial)
{
	static TMTag *tag = NULL;
	TMTag **result;
	
	if ((!sorted_tags_array) || (!sorted_tags_array->len))
		return NULL;

	if (NULL == tag)
		tag = g_new0(TMTag, 1);
	tag->name = (char *) name;
	s_sort_attrs = NULL;
	s_partial = partial;
	result = (TMTag **) bsearch(&tag, sorted_tags_array->pdata, sorted_tags_array->len
	  , sizeof(gpointer), tm_tag_compare);
	if (result)
	{
		for (; result >= (TMTag **) sorted_tags_array->pdata; -- result)
			if (0 != tm_tag_compare(&tag, (TMTag **) result))
				break;
		++ result;
	}
	s_partial = FALSE;
	return (TMTag **) result;
}
