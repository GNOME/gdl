#ifndef TM_SYMBOL_H
#define TM_SYMBOL_H

/*! \file
 The TMSymbol structure and related routines are used by TMProject to maintain a symbol
 hierarchy. The top level TMSymbol maintains a pretty simple hierarchy, consisting of
 compounds (classes and structs) and their children (member variables and functions).
*/

#include <glib.h>

#include "tm_tag.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*! This structure defines a symbol */
typedef struct _TMSymbol
{
	TMTag *tag; /*!< The tag corresponding to this symbol */
	struct _TMSymbol *parent; /*!< Parent class/struct for functions/variables */
	GSList *children; /*!< List of member functions/variables for classes/structs */
} TMSymbol;

#define TM_SYMBOL(S) ((TMSymbol *) (S))

/*! Creates a symbol tree from an array of tags.
\param tags The array of tags from which to create the symbol tree.
\return The root symbol (starting point of the symbol tree)
*/
TMSymbol *tm_symbol_tree_new(GPtrArray *tags);

/*! Given a symbol, frees it and all its children.
\param root The symbol to free.
*/
void tm_symbol_tree_free(gpointer root);

/*! Given a symbol tree and an array of tags, updates the symbol tree. Note
that the returned pointer may be different from the passed root pointer,
so don't throw it away. This is basically a convinience function that calls
tm_symbol_tree_free() and tm_symbol_tree_new().
\param root The original root symbol.
\param tags The array of tags from which to rebuild the tree.
\return The new root symbol.
*/
TMSymbol *tm_symbol_tree_update(TMSymbol *root, GPtrArray *tags);

/*! Symbol comparison function - can be used for sorting purposes. */
gint tm_symbol_compare(gconstpointer p1, gconstpointer p2);

#ifdef __cplusplus
}
#endif

#endif /* TM_SYMBOL_H */
