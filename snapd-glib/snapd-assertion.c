/*
 * Copyright (C) 2017 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include "config.h"

#include "snapd-assertion.h"

/**
 * SECTION: snapd-assertion
 * @short_description: Assertions
 * @include: snapd-glib/snapd-glib.h
 *
 * A #SnapdAssertion represents an digitally signed documents that express a
 * fact or policy by a particular authority about a particular object in the
 * snap universe. Assertions can be queried using
 * snapd_client_get_assertions_sync().
 */

/**
 * SnapdAssertion:
 *
 * #SnapdAssertion is an opaque data structure and can only be accessed
 * using the provided functions.
 */

struct _SnapdAssertion
{
    GObject parent_instance;

    GHashTable *headers;
    gchar *body;
    gchar *signature;  
};

enum 
{
    PROP_HEADERS = 1,
    PROP_BODY,
    PROP_SIGNATURE,
    PROP_LAST
};
 
G_DEFINE_TYPE (SnapdAssertion, snapd_assertion, G_TYPE_OBJECT)

/**
 * snapd_assertion_get_assertion_type:
 * @assertion: a #SnapdAssertion.
 *
 * Get the type of assertion, e.g. "account" or "model".
 *
 * Returns: an assertion type.
 */
const gchar *
snapd_assertion_get_assertion_type (SnapdAssertion *assertion)
{
    return snapd_assertion_get_header (assertion, "type");
}

/**
 * snapd_assertion_get_authority_id:
 * @assertion: a #SnapdAssertion.
 *
 * Get the authority that made the assertion.
 *
 * Returns: a authority id.
 */
const gchar *
snapd_assertion_get_authority_id (SnapdAssertion *assertion)
{
    return snapd_assertion_get_header (assertion, "authority-id");
}

/**
 * snapd_assertion_get_revision:
 * @assertion: a #SnapdAssertion.
 *
 * Get the assertion revision.
 *
 * Returns: a revision.
 */
const gchar *
snapd_assertion_get_revision (SnapdAssertion *assertion)
{
    return snapd_assertion_get_header (assertion, "revision");
}

/**
 * snapd_assertion_get_sign_key_sha3_384:
 * @assertion: a #SnapdAssertion.
 *
 * Get the encoded key id of signing key.
 *
 * Returns: encoded key id.
 */
const gchar *
snapd_assertion_get_sign_key_sha3_384 (SnapdAssertion *assertion)
{
    return snapd_assertion_get_header (assertion, "sign-key-sha3-384");
}

/**
 * snapd_assertion_get_header:
 * @assertion: a #SnapdAssertion.
 * @name: name of the header.
 *
 * Get a header from an assertion.
 *
 * Returns: (allow-none): header value or %NULL if undefined.
 */
const gchar *
snapd_assertion_get_header (SnapdAssertion *assertion, const gchar *name)
{
    g_return_val_if_fail (SNAPD_IS_ASSERTION (assertion), NULL);
    g_return_val_if_fail (name != NULL, NULL);  
    return g_hash_table_lookup (assertion->headers, name);
}

/**
 * snapd_assertion_get_headers:
 * @assertion: a #SnapdAssertion.
 *
 * Get the headers for this assertion.
 *
 * Returns: (transfer none): table of header values keyed by header name.
 */
GHashTable *
snapd_assertion_get_headers (SnapdAssertion *assertion)
{
    g_return_val_if_fail (SNAPD_IS_ASSERTION (assertion), NULL);
    return assertion->headers;
}

/**
 * snapd_assertion_get_body:
 * @assertion: a #SnapdAssertion.
 *
 * Get the body of the assertion.
 *
 * Returns: (allow-none): assertion body or %NULL.
 */
const gchar *
snapd_assertion_get_body (SnapdAssertion *assertion)
{
    g_return_val_if_fail (SNAPD_IS_ASSERTION (assertion), NULL);
    return assertion->body;
}

/**
 * snapd_assertion_get_signature:
 * @assertion: a #SnapdAssertion.
 *
 * Get the signature of the assertion.
 *
 * Returns: assertion signature.
 */
const gchar *
snapd_assertion_get_signature (SnapdAssertion *assertion)
{
    g_return_val_if_fail (SNAPD_IS_ASSERTION (assertion), NULL);
    return assertion->signature;
}

static void
snapd_assertion_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    SnapdAssertion *assertion = SNAPD_ASSERTION (object);

    switch (prop_id) {
    case PROP_HEADERS:
        g_clear_pointer (&assertion->headers, g_hash_table_unref);
        if (g_value_get_boxed (value) != NULL)
            assertion->headers = g_hash_table_ref (g_value_get_boxed (value));
        break;
    case PROP_BODY:
        g_free (assertion->body);
        assertion->body = g_strdup (g_value_get_string (value));
        break;
    case PROP_SIGNATURE:
        g_free (assertion->signature);
        assertion->signature = g_strdup (g_value_get_string (value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
snapd_assertion_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    SnapdAssertion *assertion = SNAPD_ASSERTION (object);

    switch (prop_id) {
    case PROP_HEADERS:
        g_value_set_boxed (value, assertion->headers);
        break;
    case PROP_BODY:
        g_value_set_string (value, assertion->body);
        break;
    case PROP_SIGNATURE:
        g_value_set_string (value, assertion->signature);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
snapd_assertion_finalize (GObject *object)
{
    SnapdAssertion *assertion = SNAPD_ASSERTION (object);

    g_clear_pointer (&assertion->headers, g_hash_table_unref);
    g_clear_pointer (&assertion->body, g_free);
    g_clear_pointer (&assertion->signature, g_free);  
}

static void
snapd_assertion_class_init (SnapdAssertionClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->set_property = snapd_assertion_set_property;
    gobject_class->get_property = snapd_assertion_get_property; 
    gobject_class->finalize = snapd_assertion_finalize;

    g_object_class_install_property (gobject_class,
                                     PROP_HEADERS,
                                     g_param_spec_boxed ("headers",
                                                         "headers",
                                                         "Headers for this assertion",
                                                         G_TYPE_HASH_TABLE,
                                                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_BODY,
                                     g_param_spec_string ("body",
                                                          "body",
                                                          "Assertion body",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_SIGNATURE,
                                     g_param_spec_string ("signature",
                                                          "signature",
                                                          "Assertion signature",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
snapd_assertion_init (SnapdAssertion *assertion)
{
    assertion->headers = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
}
