#include <string.h>
#include "tm_symbol.h"

static GMemChunk *sym_mem_chunk = NULL;

#define SYM_NEW(T) {\
	if (!sym_mem_chunk) \
		sym_mem_chunk = g_mem_chunk_new("TMSymbol MemChunk", sizeof(TMSymbol), 1024 \
		  , G_ALLOC_AND_FREE); \
	(T) = g_chunk_new0(TMSymbol, sym_mem_chunk);}

#define SYM_FREE(T) g_mem_chunk_free(sym_mem_chunk, (T))

#ifdef TM_DEBUG
static void tm_symbol_print(TMSymbol *sym, guint level)
{
	GSList *tmp;
	int i;

	g_assert(sym);
	for (i=0; i < level; ++i)
		fputc('\t', stderr);
	fprintf(stderr, "%s\n", (sym->tag)?sym->tag->name:"Root");
	for (tmp = sym->children; tmp; tmp = g_slist_next(tmp))
		tm_symbol_print(TM_SYMBOL(tmp->data), level + 1);
}
#endif

TMSymbol *tm_symbol_tree_new(GPtrArray *tags_array)
{
	TMSymbol *root, *sym, *parent;
	TMTag *tag;
	GPtrArray *tags;
	GSList *tmp;
	int i, cmp;

#ifdef TM_DEBUG
	g_message("Building symbol tree..");
#endif

	if ((!tags_array) || (tags_array->len <= 0))
		return NULL;

	SYM_NEW(root);
	/* Extract classes and structs and push them as children of root */
	tags = tm_tags_extract(tags_array, tm_tag_class_t | tm_tag_struct_t);
	if (tags && (tags->len > 0))
	{
		tm_tags_sort(tags, NULL, TRUE);
		for (i=0; i < tags->len; ++i)
		{
			SYM_NEW(sym);
			sym->tag = TM_TAG(tags->pdata[i]);
			sym->parent = root;
			root->children = g_slist_append(root->children, sym);
		}
#ifdef TM_DEBUG
		fprintf(stderr, "Classes & structs\n");
		tm_symbol_print(root, 0);
#endif
		g_ptr_array_free(tags, TRUE);
	}

	/* Extract functions, variables and members */
	tags = tm_tags_extract(tags_array, tm_tag_function_t | tm_tag_member_t | tm_tag_variable_t
		  | tm_tag_externvar_t);
	if ((tags) && (tags->len > 0))
	{
		for (i=0; i < tags->len; ++i)
		{
			tag = TM_TAG(tags->pdata[i]);
			SYM_NEW(sym);
			sym->tag = tag;
			if (!tag->atts.entry.scope)
			{
				sym->parent = root;
				root->children = g_slist_append(root->children, sym);
			}
			else /* Find the class/struct matching the scope */
			{
				for (tmp = root->children; tmp; tmp = g_slist_next(tmp))
				{
					parent = TM_SYMBOL(tmp->data);
					cmp = strcmp(parent->tag->name, sym->tag->atts.entry.scope);
					if (0 == cmp)
					{
						sym->parent = parent;
						parent->children = g_slist_append(parent->children, sym);
						break;
					}
					else if (0 < cmp)
						break;
				}
			}
		}
	}
	g_ptr_array_free(tags, TRUE);
#ifdef TM_DEBUG
	tm_symbol_print(root, 0);
#endif
	return root;
}

static void tm_symbol_free(TMSymbol *sym)
{
	GSList *tmp;

	if (!sym)
		return;
	if (sym->children)
	{
		for (tmp = sym->children; tmp; tmp = g_slist_next(tmp))
			tm_symbol_free(TM_SYMBOL(tmp->data));
		g_slist_free(sym->children);
		sym->children = NULL;
	}
	SYM_FREE(sym);
}

void tm_symbol_tree_free(gpointer root)
{
	if (root)
		tm_symbol_free(TM_SYMBOL(root));
}

TMSymbol *tm_symbol_tree_update(TMSymbol *root, GPtrArray *tags)
{
	if (root)
		tm_symbol_free(root);
	if ((tags) && (tags->len > 0))
		return tm_symbol_tree_new(tags);
	else
		return NULL;
}
