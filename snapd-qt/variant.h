/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#ifndef VARIANT_H
#define VARIANT_H

static QVariant
gvariant_to_qvariant (GVariant *variant)
{
    if (variant == NULL)
        return QVariant ();

    if (g_variant_is_of_type (variant, G_VARIANT_TYPE_BOOLEAN))
        return QVariant ((bool) g_variant_get_boolean (variant));
    if (g_variant_is_of_type (variant, G_VARIANT_TYPE_INT64))
        return QVariant ((qlonglong) g_variant_get_int64 (variant));
    if (g_variant_is_of_type (variant, G_VARIANT_TYPE_STRING))
        return QVariant (g_variant_get_string (variant, NULL));
    if (g_variant_is_of_type (variant, G_VARIANT_TYPE_DOUBLE))
        return QVariant (g_variant_get_double (variant));
    if (g_variant_is_of_type (variant, G_VARIANT_TYPE ("av"))) {
        QList<QVariant> list;
        GVariantIter iter;
        g_variant_iter_init (&iter, variant);
        GVariant *value;
        while (g_variant_iter_loop (&iter, "v", &value))
            list.append (gvariant_to_qvariant (value));
        return QVariant (list);
    }
    if (g_variant_is_of_type (variant, G_VARIANT_TYPE ("a{sv}"))) {
        QHash<QString, QVariant> object;
        GVariantIter iter;
        g_variant_iter_init (&iter, variant);
        const gchar *key;
        GVariant *value;
        while (g_variant_iter_loop (&iter, "{sv}", &key, &value))
            object.insert (key, gvariant_to_qvariant (value));
        return QVariant (object);
    }
    if (g_variant_is_of_type (variant, G_VARIANT_TYPE ("mv")))
        return QVariant ();

    return QVariant ();
}

#endif
