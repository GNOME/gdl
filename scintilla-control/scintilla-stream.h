#ifndef __SCINTILLA_STREAM_H__
#define __SCINTILLA_STREAM_H__

#include <bonobo/bonobo-stream.h>

#include "scintilla/ScintillaWidget.h"

BEGIN_GNOME_DECLS

struct _ScintillaStream;
typedef struct _ScintillaStream ScintillaStream;
typedef struct _ScintillaStreamPrivate BonoboStreamPrivate;

#define SCINTILLA_STREAM_TYPE        (scintilla_stream_get_type ())
#define SCINTILLA_STREAM(o)          (GTK_CHECK_CAST ((o), SCINTILLA_STREAM_TYPE, ScintillaStream))
#define SCINTILLA_STREAM_CLASS(k)    (GTK_CHECK_CLASS_CAST((k), SCINTILLA_STREAM_TYPE, ScintillaStreamClass))
#define BONOBO_IS_STREAM_MEM(o)       (GTK_CHECK_TYPE ((o), SCINTILLA_STREAM_TYPE))
#define BONOBO_IS_STREAM_MEM_CLASS(k) (GTK_CHECK_CLASS_TYPE ((k), SCINTILLA_STREAM_TYPE))

struct _ScintillaStream {
    BonoboStream  stream;
    
    ScintillaObject *sci;

    ScintillaStreamPrivate *priv;
};

typedef struct {
    BonoboStreamClass parent_class;
    char           *(*get_buffer) (ScintillaStream *stream_mem);
    size_t          (*get_size)   (ScintillaStream *stream_mem);
} ScintillaStreamClass;

GtkType          scintilla_stream_get_type   (void);
ScintillaStream *scintilla_stream_construct  (ScintillaStream *stream_mem,
					      Bonobo_Stream    corba_stream,
					      ScintillaObject *sci,
					      gboolean         read_only);
BonoboStream    *scintilla_stream_create     (ScintillaObject *obj,
					      gboolean read_only);

END_GNOME_DECLS

#endif /* __SCINTILLA_STREAM_H__ */
