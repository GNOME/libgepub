/* GepubDoc
 *
 * Copyright (C) 2011 Daniel Garcia <danigm@wadobo.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <config.h>
#include <gio/gio.h>
#include <libxml/tree.h>
#include <libxml/HTMLparser.h>
#include <string.h>

#include "gepub-doc.h"


// Rust
void      *epub_new(char *path);
void       epub_destroy(void *doc);
void      *epub_get_resource(void *doc, const char *path, int *size);
void      *epub_get_resource_by_id(void *doc, const char *id, int *size);
void      *epub_get_metadata(void *doc, const char *mdata);
void      *epub_get_resource_mime(void *doc, const char *path);
void      *epub_get_resource_mime_by_id(void *doc, const char *id);
void      *epub_get_current_mime(void *doc);
void      *epub_get_current(void *doc, int *size);
void      *epub_get_current_with_epub_uris(void *doc, int *size);
void       epub_set_page(void *doc, guint page);
guint      epub_get_num_pages(void *doc);
guint      epub_get_page(void *doc);
gboolean   epub_next_page(void *doc);
gboolean   epub_prev_page(void *doc);
void      *epub_get_cover(void *doc);
void      *epub_resource_path(void *doc, const char *id);
void      *epub_current_path(void *doc);
void      *epub_current_id(void *doc);
void      *epub_get_resources(void *doc);
guint      epub_resources_get_length(void *er);

gchar     *epub_resources_get_id(void *er, gint i);
gchar     *epub_resources_get_mime(void *er, gint i);
gchar     *epub_resources_get_path(void *er, gint i);


static GQuark
gepub_error_quark (void)
{
    static GQuark q = 0;
    if (q == 0)
        q = g_quark_from_string ("gepub-quark");
    return q;
}

/**
 * GepubDocError:
 * @GEPUB_ERROR_INVALID: Invalid file
 *
 * Common errors that may be reported by GepubDoc.
 */
typedef enum {
    GEPUB_ERROR_INVALID = 0,  /*< nick=Invalid >*/
} GepubDocError;



static void gepub_doc_initable_iface_init (GInitableIface *iface);

struct _GepubDoc {
    GObject parent;
    gchar *path;
    void *rust_epub_doc;
};

struct _GepubDocClass {
    GObjectClass parent_class;
};

enum {
    PROP_0,
    PROP_PATH,
    PROP_CHAPTER,
    NUM_PROPS
};

static GParamSpec *properties[NUM_PROPS] = { NULL, };

G_DEFINE_TYPE_WITH_CODE (GepubDoc, gepub_doc, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE, gepub_doc_initable_iface_init))

static void
gepub_resource_free (GepubResource *res)
{
    g_free (res->mime);
    g_free (res->uri);
    g_free (res);
}

static void
gepub_doc_finalize (GObject *object)
{
    GepubDoc *doc = GEPUB_DOC (object);

    epub_destroy (doc->rust_epub_doc);
    g_clear_pointer (&doc->path, g_free);

    G_OBJECT_CLASS (gepub_doc_parent_class)->finalize (object);
}

static void
gepub_doc_set_property (GObject      *object,
            guint         prop_id,
            const GValue *value,
            GParamSpec   *pspec)
{
    GepubDoc *doc = GEPUB_DOC (object);

    switch (prop_id) {
    case PROP_PATH:
        doc->path = g_value_dup_string (value);
        break;
    case PROP_CHAPTER:
        gepub_doc_set_chapter (doc, g_value_get_int (value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
gepub_doc_get_property (GObject    *object,
            guint       prop_id,
            GValue     *value,
            GParamSpec *pspec)
{
    GepubDoc *doc = GEPUB_DOC (object);

    switch (prop_id) {
    case PROP_PATH:
        g_value_set_string (value, doc->path);
        break;
    case PROP_CHAPTER:
        g_value_set_int (value, gepub_doc_get_chapter (doc));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
gepub_doc_init (GepubDoc *doc)
{
}

static void
gepub_doc_class_init (GepubDocClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = gepub_doc_finalize;
    object_class->set_property = gepub_doc_set_property;
    object_class->get_property = gepub_doc_get_property;

    properties[PROP_PATH] =
        g_param_spec_string ("path",
                             "Path",
                             "Path to the EPUB document",
                             NULL,
                             G_PARAM_READWRITE |
                             G_PARAM_CONSTRUCT_ONLY |
                             G_PARAM_STATIC_STRINGS);
    properties[PROP_CHAPTER] =
        g_param_spec_int ("chapter",
                          "Current chapter",
                          "The current chapter index",
                          -1, G_MAXINT, 0,
                          G_PARAM_READWRITE |
                          G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties (object_class, NUM_PROPS, properties);
}

static gboolean
gepub_doc_initable_init (GInitable     *initable,
                         GCancellable  *cancellable,
                         GError       **error)
{
    GepubDoc *doc = GEPUB_DOC (initable);

    g_assert (doc->path != NULL);
    doc->rust_epub_doc = epub_new (doc->path);
    if (!doc->rust_epub_doc) {
        if (error != NULL) {
            g_set_error (error, gepub_error_quark (), GEPUB_ERROR_INVALID,
                         "Invalid epub file: %s", doc->path);
        }
        return FALSE;
    }

    return TRUE;
}

static void
gepub_doc_initable_iface_init (GInitableIface *iface)
{
    iface->init = gepub_doc_initable_init;
}

/**
 * gepub_doc_new:
 * @path: the epub doc path
 * @error: (nullable): Error
 *
 * Returns: (transfer full): the new GepubDoc created
 */
GepubDoc *
gepub_doc_new (const gchar *path, GError **error)
{
    return g_initable_new (GEPUB_TYPE_DOC,
                           NULL, error,
                           "path", path,
                           NULL);
}

/**
 * gepub_doc_get_metadata:
 * @doc: a #GepubDoc
 * @mdata: a metadata name string, GEPUB_META_TITLE for example
 *
 * Returns: (transfer full): metadata string
 */
gchar *
gepub_doc_get_metadata (GepubDoc *doc, const gchar *mdata)
{
    g_return_val_if_fail (GEPUB_IS_DOC (doc), NULL);
    g_return_val_if_fail (mdata != NULL, NULL);

    return epub_get_metadata (doc->rust_epub_doc, mdata);
}

/**
 * gepub_doc_get_resources:
 * @doc: a #GepubDoc
 *
 * Returns: (element-type utf8 Gepub.Resource) (transfer full): doc resource table
 */
GHashTable *
gepub_doc_get_resources (GepubDoc *doc)
{
    g_return_val_if_fail (GEPUB_IS_DOC (doc), NULL);

    GHashTable *resources = g_hash_table_new_full (g_str_hash,
                                                   g_str_equal,
                                                   (GDestroyNotify)g_free,
                                                   (GDestroyNotify)gepub_resource_free);

    void *res = epub_get_resources (doc->rust_epub_doc);
    guint l = epub_resources_get_length (res);
    gint i = 0;

    for (i=0; i<l; i++) {
        gchar *key = epub_resources_get_id (res, i);
        GepubResource *r = g_malloc (sizeof (GepubResource));
        r->uri = epub_resources_get_path (res, i);
        r->mime = epub_resources_get_mime (res, i);
        g_hash_table_insert (resources, key, r);
    }

    return resources;
}

/**
 * gepub_doc_get_resource_by_id:
 * @doc: a #GepubDoc
 * @id: the resource id
 *
 * Returns: (transfer full): the resource content
 */
GBytes *
gepub_doc_get_resource_by_id (GepubDoc *doc, const gchar *id)
{
    g_return_val_if_fail (GEPUB_IS_DOC (doc), NULL);
    g_return_val_if_fail (id != NULL, NULL);

    int size = 0;
    guint8 *data = epub_get_resource_by_id (doc->rust_epub_doc, id, &size);
    return g_bytes_new_take (data, size);
}

/**
 * gepub_doc_get_resource:
 * @doc: a #GepubDoc
 * @path: the resource path
 *
 * Returns: (transfer full): the resource content
 */
GBytes *
gepub_doc_get_resource (GepubDoc *doc, const gchar *path)
{
    g_return_val_if_fail (GEPUB_IS_DOC (doc), NULL);
    g_return_val_if_fail (path != NULL, NULL);

    int size = 0;
    guint8 *data = epub_get_resource (doc->rust_epub_doc, path, &size);
    return g_bytes_new_take (data, size);
}

/**
 * gepub_doc_get_resource_mime_by_id:
 * @doc: a #GepubDoc
 * @id: the resource id
 *
 * Returns: (transfer full): the resource content
 */
gchar *
gepub_doc_get_resource_mime_by_id (GepubDoc *doc, const gchar *id)
{
    g_return_val_if_fail (GEPUB_IS_DOC (doc), NULL);
    g_return_val_if_fail (id != NULL, NULL);

    return epub_get_resource_mime_by_id (doc->rust_epub_doc, id);
}

/**
 * gepub_doc_get_resource_mime:
 * @doc: a #GepubDoc
 * @path: the resource path
 *
 * Returns: (transfer full): the resource mime
 */
gchar *
gepub_doc_get_resource_mime (GepubDoc *doc, const gchar *path)
{
    g_return_val_if_fail (GEPUB_IS_DOC (doc), NULL);
    g_return_val_if_fail (path != NULL, NULL);

    return epub_get_resource_mime (doc->rust_epub_doc, path);
}

/**
 * gepub_doc_get_current_mime:
 * @doc: a #GepubDoc
 *
 * Returns: (transfer full): the current resource mime
 */
gchar *
gepub_doc_get_current_mime (GepubDoc *doc)
{
    g_return_val_if_fail (GEPUB_IS_DOC (doc), NULL);

    return epub_get_current_mime (doc->rust_epub_doc);
}

/**
 * gepub_doc_get_current:
 * @doc: a #GepubDoc
 *
 * Returns: (transfer full): the current chapter data
 */
GBytes *
gepub_doc_get_current (GepubDoc *doc)
{
    g_return_val_if_fail (GEPUB_IS_DOC (doc), NULL);

    int size = 0;
    guint8 *data = epub_get_current (doc->rust_epub_doc, &size);
    return g_bytes_new_take (data, size);
}

/**
 * gepub_doc_get_current_with_epub_uris:
 * @doc: a #GepubDoc
 *
 * Returns: (transfer full): the current chapter
 * data, with resource uris renamed so they have the epub:/// prefix and all
 * are relative to the root file
 */
GBytes *
gepub_doc_get_current_with_epub_uris (GepubDoc *doc)
{
    g_return_val_if_fail (GEPUB_IS_DOC (doc), NULL);

    int size = 0;
    guint8 *data = epub_get_current_with_epub_uris (doc->rust_epub_doc, &size);
    return g_bytes_new_take (data, size);
}

/**
 * gepub_doc_go_next:
 * @doc: a #GepubDoc
 *
 * Returns: TRUE on success, FALSE if there's no next chapter
 */
gboolean
gepub_doc_go_next (GepubDoc *doc)
{
    g_return_val_if_fail (GEPUB_IS_DOC (doc), FALSE);
    gboolean isok = epub_next_page (doc->rust_epub_doc);
    if (isok) {
        g_object_notify_by_pspec (G_OBJECT (doc), properties[PROP_CHAPTER]);
    }
    return isok;
}

/**
 * gepub_doc_go_prev:
 * @doc: a #GepubDoc
 *
 * Returns: TRUE on success, FALSE if there's no previous chapter
 */
gboolean
gepub_doc_go_prev (GepubDoc *doc)
{
    g_return_val_if_fail (GEPUB_IS_DOC (doc), FALSE);
    gboolean isok = epub_prev_page (doc->rust_epub_doc);
    if (isok) {
        g_object_notify_by_pspec (G_OBJECT (doc), properties[PROP_CHAPTER]);
    }
    return isok;
}

/**
 * gepub_doc_get_n_chapters:
 * @doc: a #GepubDoc
 *
 * Returns: the number of chapters in the document
 */
int
gepub_doc_get_n_chapters (GepubDoc *doc)
{
    g_return_val_if_fail (GEPUB_IS_DOC (doc), 0);
    return epub_get_num_pages (doc->rust_epub_doc);
}

/**
 * gepub_doc_get_chapter:
 * @doc: a #GepubDoc
 *
 * Returns: the current chapter index, starting from 0
 */
int
gepub_doc_get_chapter (GepubDoc *doc)
{
    g_return_val_if_fail (GEPUB_IS_DOC (doc), 0);
    return epub_get_page (doc->rust_epub_doc);
}

/**
 * gepub_doc_set_chapter:
 * @doc: a #GepubDoc
 * @index: the index of the new chapter
 *
 * Sets the document current chapter to @index.
 */
void
gepub_doc_set_chapter (GepubDoc *doc,
                       gint      index)
{
    g_return_if_fail (GEPUB_IS_DOC (doc));
    g_return_if_fail (index >= 0 && index <= gepub_doc_get_n_chapters (doc));
    g_object_notify_by_pspec (G_OBJECT (doc), properties[PROP_CHAPTER]);
    epub_set_page (doc->rust_epub_doc, index);
}

/**
 * gepub_doc_get_cover:
 * @doc: a #GepubDoc
 *
 * Returns: (transfer full): cover file path to retrieve with
 * gepub_doc_get_resource
 */
gchar *
gepub_doc_get_cover (GepubDoc *doc)
{
    g_return_val_if_fail (GEPUB_IS_DOC (doc), NULL);
    return epub_get_cover (doc->rust_epub_doc);
}

/**
 * gepub_doc_get_resource_path:
 * @doc: a #GepubDoc
 * @id: the resource id
 *
 * Returns: (transfer full): the resource path
 */
gchar *
gepub_doc_get_resource_path (GepubDoc *doc, const gchar *id)
{
    g_return_val_if_fail (GEPUB_IS_DOC (doc), NULL);
    g_return_val_if_fail (id != NULL, NULL);

    return epub_resource_path(doc->rust_epub_doc, id);
}

/**
 * gepub_doc_get_current_path:
 * @doc: a #GepubDoc
 *
 * Returns: (transfer full): the current resource path
 */
gchar *
gepub_doc_get_current_path (GepubDoc *doc)
{
    g_return_val_if_fail (GEPUB_IS_DOC (doc), NULL);
    return epub_current_path (doc->rust_epub_doc);
}

/**
 * gepub_doc_get_current_id:
 * @doc: a #GepubDoc
 *

 * Returns: (transfer none): the current resource id
 */
const gchar *
gepub_doc_get_current_id (GepubDoc *doc)
{
    g_return_val_if_fail (GEPUB_IS_DOC (doc), NULL);
    return epub_current_id (doc->rust_epub_doc);
}
