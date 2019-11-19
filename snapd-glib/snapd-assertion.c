/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "snapd-assertion.h"

/**
 * SECTION: snapd-assertion
 * @short_description: Assertions
 * @include: snapd-glib/snapd-glib.h
 *
 * A #SnapdAssertion represents an digitally signed documents that express a
 * fact or policy by a particular authority about a particular object in the
 * snap universe. Assertions can be queried using
 * snapd_client_get_assertions_sync(). New assertions can be provided using
 * snapd_client_add_assertions_sync().
 */

/**
 * SnapdAssertion:
 *
 * #SnapdAssertion contains information about a Snap assertion.
 *
 * Assertions are digitally signed documents that allow Snaps to have secure
 * trust and control features.
 *
 * Since: 1.0
 */

struct _SnapdAssertion
{
    GObject parent_instance;

    gchar *content;
};

enum
{
    PROP_CONTENT = 1,
    PROP_LAST
};

G_DEFINE_TYPE (SnapdAssertion, snapd_assertion, G_TYPE_OBJECT)

/**
 * snapd_assertion_new:
 * @content: the text content of the assertion.
 *
 * Create a new assertion.
 *
 * Returns: a new #SnapdAssertion
 *
 * Since: 1.0
 **/
SnapdAssertion *
snapd_assertion_new (const gchar *content)
{
    SnapdAssertion *self = g_object_new (SNAPD_TYPE_ASSERTION, NULL);
    self->content = g_strdup (content);

    return self;
}

static gboolean
get_header (const gchar *content, gsize *offset, gsize *name_start, gsize *name_length, gsize *value_start, gsize *value_length)
{
    /* Name separated from value by colon */
    if (name_start != NULL)
        *name_start = *offset;
    while (content[*offset] && content[*offset] != ':' && content[*offset] != '\n')
        (*offset)++;
    if (content[*offset] == '\0' || content[*offset] != ':')
        return FALSE;
    if (name_start != NULL && name_length != NULL)
        *name_length = *offset - *name_start;
    (*offset)++;

    /* Value terminated by newline */
    while (content[*offset] && content[*offset] != '\n' && isspace (content[*offset]))
        (*offset)++;
    if (value_start != NULL)
        *value_start = *offset;
    while (content[*offset] && content[*offset] != '\n')
        (*offset)++;
    if (content[*offset] == '\0' || content[*offset] != '\n')
        return FALSE;
    (*offset)++;

    /* Value continued by lines starting with spaces */
    while (content[*offset] && content[*offset] == ' ') {
        while (content[*offset]) {
           if (content[*offset] == '\n') {
                (*offset)++;
                break;
            }
            (*offset)++;
        }
    }
    if (value_start != NULL && value_length != NULL)
        *value_length = *offset - *value_start - 1;

    return TRUE;
}

/**
 * snapd_assertion_get_headers:
 * @assertion: a #SnapdAssertion.
 *
 * Get the headers provided by this assertion.
 *
 * Returns: (transfer full) (array zero-terminated=1): array of header names.
 *
 * Since: 1.0
 */
GStrv
snapd_assertion_get_headers (SnapdAssertion *self)
{
    g_return_val_if_fail (SNAPD_IS_ASSERTION (self), NULL);

    gsize offset = 0;
    g_autoptr(GPtrArray) headers = g_ptr_array_new ();
    while (TRUE) {
        /* Headers terminated by double newline or EOF */
        gsize name_start, name_length;
        if (self->content[offset] == '\0' ||
            self->content[offset] == '\n' ||
            !get_header (self->content, &offset, &name_start, &name_length, NULL, NULL))
            break;

        g_ptr_array_add (headers, g_strndup (self->content + name_start, name_length));
    }
    g_ptr_array_add (headers, NULL);

    return g_steal_pointer ((GStrv *)&headers->pdata);
}

/**
 * snapd_assertion_get_header:
 * @assertion: a #SnapdAssertion.
 * @name: name of the header.
 *
 * Get a header from an assertion.
 *
 * Returns: (transfer full) (allow-none): header value or %NULL if undefined.
 *
 * Since: 1.0
 */
gchar *
snapd_assertion_get_header (SnapdAssertion *self, const gchar *name)
{
    g_return_val_if_fail (SNAPD_IS_ASSERTION (self), NULL);
    g_return_val_if_fail (name != NULL, NULL);

    gsize offset = 0;
    while (TRUE) {
        /* Headers terminated by double newline or EOF */
        if (self->content[offset] == '\0' || self->content[offset] == '\n')
            return NULL;

        gsize name_start, name_length, value_start, value_length;
        if (!get_header (self->content, &offset, &name_start, &name_length, &value_start, &value_length))
            return NULL;

        /* Return value if header we're looking for */
        if (strncmp (self->content + name_start, name, name_length) == 0)
            return g_strndup (self->content + value_start, value_length);
    }

    return NULL;
}

static gsize
get_headers_length (SnapdAssertion *self)
{
    /* Headers terminated by double newline */
    gchar *divider = strstr (self->content, "\n\n");
    if (divider == NULL)
        return 0;

    return divider - self->content;
}

static gsize
get_body_length (SnapdAssertion *self)
{
    g_autofree gchar *body_length_header = snapd_assertion_get_header (self, "body-length");
    if (body_length_header == NULL)
        return 0;

    return strtoul (body_length_header, NULL, 10);
}

/**
 * snapd_assertion_get_body:
 * @assertion: a #SnapdAssertion.
 *
 * Get the body of the assertion.
 *
 * Returns: (transfer full) (allow-none): assertion body or %NULL.
 *
 * Since: 1.0
 */
gchar *
snapd_assertion_get_body (SnapdAssertion *self)
{
    g_return_val_if_fail (SNAPD_IS_ASSERTION (self), NULL);

    gsize body_length = get_body_length (self);
    if (body_length == 0)
        return NULL;

    return g_strndup (self->content + get_headers_length (self) + 2, body_length);
}

/**
 * snapd_assertion_get_signature:
 * @assertion: a #SnapdAssertion.
 *
 * Get the signature of the assertion.
 *
 * Returns: assertion signature.
 *
 * Since: 1.0
 */
gchar *
snapd_assertion_get_signature (SnapdAssertion *self)
{
    g_return_val_if_fail (SNAPD_IS_ASSERTION (self), NULL);

    gsize body_length = get_body_length (self);
    if (body_length > 0)
        return g_strdup (self->content + get_headers_length (self) + 2 + body_length + 2);
    else
        return g_strdup (self->content + get_headers_length (self) + 2);
}

static void
snapd_assertion_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    SnapdAssertion *self = SNAPD_ASSERTION (object);

    switch (prop_id) {
    case PROP_CONTENT:
        g_free (self->content);
        self->content = g_strdup (g_value_get_string (value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
snapd_assertion_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    SnapdAssertion *self = SNAPD_ASSERTION (object);

    switch (prop_id) {
    case PROP_CONTENT:
        g_value_set_string (value, self->content);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
snapd_assertion_finalize (GObject *object)
{
    SnapdAssertion *self = SNAPD_ASSERTION (object);

    g_clear_pointer (&self->content, g_free);

    G_OBJECT_CLASS (snapd_assertion_parent_class)->finalize (object);
}

static void
snapd_assertion_class_init (SnapdAssertionClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->set_property = snapd_assertion_set_property;
    gobject_class->get_property = snapd_assertion_get_property;
    gobject_class->finalize = snapd_assertion_finalize;

    g_object_class_install_property (gobject_class,
                                     PROP_CONTENT,
                                     g_param_spec_string ("content",
                                                          "content",
                                                          "Assertion content",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
snapd_assertion_init (SnapdAssertion *self)
{
}
