#include <config.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <libgnome/gnome-defs.h>
#include <libgnome/gnome-util.h>
#include <bonobo/bonobo-stream-memory.h>
#include <errno.h>

static BonoboStreamClass *scintilla_stream_parent_class;

static Bonobo_StorageInfo*
sci_get_info (BonoboStream *stream,
	      const Bonobo_StorageInfoFields mask,
	      CORBA_Environment *ev)
{
    g_warning ("Not implemented");
	
    return CORBA_OBJECT_NIL;
}

static void
sci_set_info (BonoboStream *stream,
	      const Bonobo_StorageInfo *info,
	      const Bonobo_StorageInfoFields mask,
	      CORBA_Environment *ev)
{
    g_warning ("Not implemented");
}

static void
sci_truncate (BonoboStream *stream,
	      const CORBA_long new_size, 
	      CORBA_Environment *ev)
{
#if 0
    ScintillaStream *sci_stream = SCINTILLA_STREAM (stream);
    void *newp;
	
    if (sci_stream->read_only)
	return;

    newp = g_realloc (sci_stream->buffer, new_size);
    if (!newp) {
	CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
			     ex_Bonobo_Stream_NoPermission, NULL);
	return;
    }

    sci_stream->buffer = newp;
    sci_stream->size = new_size;

    if (sci_stream->pos > new_size)
	sci_stream->pos = new_size;
#endif
    g_warning ("Not implemented");
}

static void
sci_write (BonoboStream *stream, const Bonobo_Stream_iobuf *buffer,
	   CORBA_Environment *ev)
{
    ScintillaStream *sci_stream = SCINTILLA_STREAM (stream);
    long len = buffer->_length;
#if 0
    if (sci_stream->read_only){
	g_warning ("Should signal an exception here");
	return;
    }

    if (sci_stream->pos + len > sci_stream->size){
	if (sci_stream->resizable){
	    sci_stream->size = sci_stream->pos + len;
	    sci_stream->buffer = g_realloc (sci_stream->buffer, sci_stream->size);
	} else {
	    sci_truncate (stream, sci_stream->pos + len, ev);
	    g_warning ("Should check for an exception here");
	}
    }

    if (sci_stream->pos + len > sci_stream->size)
	len = sci_stream->size - sci_stream->pos;
	
    memcpy (sci_stream->buffer + sci_stream->pos, buffer->_buffer, len);
    sci_stream->pos += len;
#endif	
    return;
}

static void
sci_read (BonoboStream *stream, CORBA_long count,
	  Bonobo_Stream_iobuf ** buffer,
	  CORBA_Environment *ev)
{
    ScintillaStream *sci_stream = SCINTILLA_STREAM (stream);
    TextRange tr;
    long pos = scintilla_send_message (sci, SCI_GETCURRENTPOS, 0, 0);
    long len = scintilla_send_message (sci, SCI_GETTEXTLENGTH, 0, 0);

    if (len - pos > count) {
	count = len - pos;
    }

    *buffer = Bonobo_Stream_iobuf__alloc ();
    CORBA_sequence_set_release (*buffer, TRUE);
    (*buffer)->_buffer = CORBA_sequence_CORBA_octet_allocbuf (count);
    (*buffer)->_length = count;

    tr.chrg.cpMin = pos;
    tr.chrg.cpMax = pos + count;
    tr.lpstrText = (char*)(*buffer)->buffer;
    
    scintilla_send_message (sci, SCI_GETTEXTRANGE, 0, (long)&tr);
}

static CORBA_long
sci_seek (BonoboStream *stream,
	  CORBA_long offset, Bonobo_Stream_SeekType whence,
	  CORBA_Environment *ev)
{
    ScintillaStream *sci_stream = SCINTILLA_STREAM (stream);
    long pos = scintilla_send_message (sci, SCI_GETCURRENTPOS, 0, 0);

    switch (whence){
    case Bonobo_Stream_SEEK_SET:
	pos = offset;
	break;

    case Bonobo_Stream_SEEK_CUR:
	pos = pos + offset;
	break;

    case Bonobo_Stream_SEEK_END:
	pos = pos + offset;
	break;

    default:
	g_warning ("Signal exception");
    }

    scintilla_send_message (sci, SCI_SETCURRENTPOS, pos, 0);
    pos = scintilla_send_message (sci, SCI_GETCURRENTPOS, 0, 0);

    return pos;
}

static void
sci_copy_to  (BonoboStream *stream,
	      const CORBA_char *dest,
	      const CORBA_long bytes,
	      CORBA_long *read,
	      CORBA_long *written,
	      CORBA_Environment *ev)
{
    ScintillaStream *sci_stream = SCINTILLA_STREAM (stream);
#if 0
    gint fd_out;
    gint w;
	
    *read = sci_stream->size - sci_stream->pos;
    *written = 0;
	
    /* create the output file */
    fd_out = creat(dest, 0666);
    if (fd_out == -1) {
	g_warning ("unable to create output file");
	return;
    }
	
    /* write the memory stream buffer to the output file */
    do {
	w = write (fd_out, sci_stream->buffer, *read);
    } while (w == -1 && errno == EINTR);
	
    if (w != -1)
	*written = w;
    else if (errno != EINTR) {
	/* should probably do something to signal an error here */
	g_warning ("ouput file write failed");
    }
	
	
    close(fd_out);
#endif
}

static void
sci_commit (BonoboStream *stream,
	    CORBA_Environment *ev)
{
    CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
			 ex_Bonobo_Stream_NotSupported, NULL);
}

static void
sci_revert (BonoboStream *stream,
	    CORBA_Environment *ev)
{
    CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
			 ex_Bonobo_Stream_NotSupported, NULL);
}

static void
sci_destroy (GtkObject *object)
{
    ScintillaStream *sci_stream = SCINTILLA_STREAM (object);	
	
    GTK_OBJECT_CLASS (scintilla_stream_parent_class)->destroy (object);
}

static void
scintilla_stream_class_init (ScintillaStreamClass *klass)
{
    GtkObjectClass *object_class = (GtkObjectClass *) klass;
    BonoboStreamClass *sclass = BONOBO_STREAM_CLASS (klass);
	
    scintilla_stream_parent_class = gtk_type_class (bonobo_stream_get_type ());

    object_class->destroy = sci_destroy;
	
    sclass->get_info  = sci_get_info;
    sclass->set_info  = sci_set_info;
    sclass->write     = sci_write;
    sclass->read      = sci_read;
    sclass->seek      = sci_seek;
    sclass->truncate  = sci_truncate;
    sclass->copy_to   = sci_copy_to;
    sclass->commit    = sci_commit;
    sclass->revert    = sci_revert;
}

GtkType
scintilla_stream_get_type (void)
{
    static GtkType type = 0;

    if (!type){
	GtkTypeInfo info = {
	    "ScintillaStream",
	    sizeof (ScintillaStream),
	    sizeof (ScintillaStreamClass),
	    (GtkClassInitFunc) scintilla_stream_class_init,
	    (GtkObjectInitFunc) NULL,
	    NULL, /* reserved 1 */
	    NULL, /* reserved 2 */
	    (GtkClassInitFunc) NULL
	};

	type = gtk_type_unique (bonobo_stream_get_type (), &info);
    }

    return type;
}

ScintillaStream *
scintilla_stream_construct (ScintillaStream *sci_stream,
			    Bonobo_Stream    corba_stream,
			    ScintillaObject *sci,
			    gboolean         read_only)
{
    g_return_val_if_fail (corba_stream != CORBA_OBJECT_NIL, NULL);
    g_return_val_if_fail (BONOBO_IS_SCI_STREAM (sci_stream), NULL);

    sci_stream->sci = sci;
    sci_stream->read_only = read_only;

    return SCINTILLA_STREAM (
			     bonobo_object_construct (BONOBO_OBJECT (sci_stream),
						      corba_stream));
}

BonoboStream *
scintilla_stream_create (ScintillaOjbect *obj,
			 gboolean read_only)
{
    ScintillaStream *sci_stream;
    Bonobo_Stream corba_stream;
    
    sci_stream = gtk_type_new (scintilla_stream_get_type ());
    if (sci_stream == NULL)
	return NULL;
    
    corba_stream = bonobo_stream_corba_object_create (
						      BONOBO_OBJECT (sci_stream));
    
    if (corba_stream == CORBA_OBJECT_NIL) {
	bonobo_object_unref (BONOBO_OBJECT (sci_stream));
	return NULL;
    }

    return BONOBO_STREAM (scintilla_stream_construct (
						      sci_stream, 
						      corba_stream, buffer, 
						      size,
						      read_only));
}
