/*
 * Copyright (C) 2016 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include "config.h"

#include "snapd-payment-method.h"

/**
 * SECTION:snapd-payment-method
 * @short_description: Payment method class
 * @include: snapd-glib/snapd-glib.h
 *
 * A #SnapdPaymentMethod is a description of a payment method returned from
 * snapd. Supported payment methods are requested using
 * snapd_client_get_payment_methods_sync().
 */

struct _SnapdPaymentMethod
{
    GObject parent_instance;

    gchar *backend_id;
    gchar **currencies;
    gchar *description;
    gint64 id;
    gboolean preferred;
    gboolean requires_interaction;
};

enum 
{
    PROP_BACKEND_ID = 1,
    PROP_CURRENCIES,
    PROP_DESCRIPTION,
    PROP_ID,
    PROP_PREFERRED,
    PROP_REQUIRES_INTERACTION,
    PROP_LAST
};
 
G_DEFINE_TYPE (SnapdPaymentMethod, snapd_payment_method, G_TYPE_OBJECT)

/**
 * snapd_payment_method_get_backend_id:
 * @payment_method: a #SnapdPaymentMethod.
 *
 * Get the backend ID for this payment method.
 *
 * Returns: a backend ID.
 */
const gchar *
snapd_payment_method_get_backend_id (SnapdPaymentMethod *payment_method)
{
    g_return_val_if_fail (SNAPD_IS_PAYMENT_METHOD (payment_method), FALSE);
    return payment_method->backend_id;
}

/**
 * snapd_payment_method_get_currencies:
 * @payment_method: a #SnapdPaymentMethod.
 *
 * Get the currencies this payment method can process.
 *
 * Returns: (transfer none) (array zero-terminated=1): an array of ISO 4217
 * currency codes.
 */
gchar **
snapd_payment_method_get_currencies (SnapdPaymentMethod *payment_method)
{
    g_return_val_if_fail (SNAPD_IS_PAYMENT_METHOD (payment_method), FALSE);
    return payment_method->currencies;
}

/**
 * snapd_payment_method_get_description:
 * @payment_method: a #SnapdPaymentMethod.
 *
 * Get the description for this payment method.
 *
 * Returns: description text.
 */
const gchar *
snapd_payment_method_get_description (SnapdPaymentMethod *payment_method)
{
    g_return_val_if_fail (SNAPD_IS_PAYMENT_METHOD (payment_method), FALSE);
    return payment_method->description;
}

/**
 * snapd_payment_method_get_id:
 * @payment_method: a #SnapdPaymentMethod.
 *
 * Get the ID for this payment method.
 *
 * Returns: an ID.
 */
gint64
snapd_payment_method_get_id (SnapdPaymentMethod *payment_method)
{
    g_return_val_if_fail (SNAPD_IS_PAYMENT_METHOD (payment_method), FALSE);
    return payment_method->id;
}

/**
 * snapd_payment_method_get_preferred:
 * @payment_method: a #SnapdPaymentMethod.
 *
 * Get if this is the preferred payment method.
 *
 * Returns: %TRUE if this payment method is the preferred one.
 */
gboolean
snapd_payment_method_get_preferred (SnapdPaymentMethod *payment_method)
{
    g_return_val_if_fail (SNAPD_IS_PAYMENT_METHOD (payment_method), FALSE);
    return payment_method->preferred;
}

/**
 * snapd_payment_method_get_requires_interaction:
 * @payment_method: a #SnapdPaymentMethod.
 *
 * Get if this payment method requires interaction to use.
 *
 * Returns: %TRUE if this method requires interaction.
 */
gboolean
snapd_payment_method_get_requires_interaction (SnapdPaymentMethod *payment_method)
{
    g_return_val_if_fail (SNAPD_IS_PAYMENT_METHOD (payment_method), FALSE);
    return payment_method->requires_interaction;
}

static void
snapd_payment_method_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    SnapdPaymentMethod *payment_method = SNAPD_PAYMENT_METHOD (object);

    switch (prop_id) {
    case PROP_BACKEND_ID:
        g_free (payment_method->backend_id);
        payment_method->backend_id = g_strdup (g_value_get_string (value));
        break;
    case PROP_CURRENCIES:
        g_strfreev (payment_method->currencies);
        payment_method->currencies = g_strdupv (g_value_get_boxed (value));
        break;
    case PROP_DESCRIPTION:
        g_free (payment_method->description);
        payment_method->description = g_strdup (g_value_get_string (value));
        break;
    case PROP_ID:
        payment_method->id = g_value_get_int64 (value);
        break;
    case PROP_PREFERRED:
        payment_method->preferred = g_value_get_boolean (value);
        break;
    case PROP_REQUIRES_INTERACTION:
        payment_method->requires_interaction = g_value_get_boolean (value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
snapd_payment_method_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    SnapdPaymentMethod *payment_method = SNAPD_PAYMENT_METHOD (object);

    switch (prop_id) {
    case PROP_BACKEND_ID:
        g_value_set_string (value, payment_method->backend_id);
        break;
    case PROP_CURRENCIES:
        g_value_set_boxed (value, payment_method->currencies);
        break;
    case PROP_DESCRIPTION:
        g_value_set_string (value, payment_method->description);
        break;
    case PROP_ID:
        g_value_set_int64 (value, payment_method->id);
        break;
    case PROP_PREFERRED:
        g_value_set_boolean (value, payment_method->preferred);
        break;
    case PROP_REQUIRES_INTERACTION:
        g_value_set_boolean (value, payment_method->requires_interaction);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
snapd_payment_method_finalize (GObject *object)
{
    SnapdPaymentMethod *payment_method = SNAPD_PAYMENT_METHOD (object);

    g_clear_pointer (&payment_method->backend_id, g_free);
    g_strfreev (payment_method->currencies);
    payment_method->currencies = NULL;
    g_clear_pointer (&payment_method->description, g_free);
}

static void
snapd_payment_method_class_init (SnapdPaymentMethodClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->set_property = snapd_payment_method_set_property;
    gobject_class->get_property = snapd_payment_method_get_property; 
    gobject_class->finalize = snapd_payment_method_finalize;

    g_object_class_install_property (gobject_class,
                                     PROP_BACKEND_ID,
                                     g_param_spec_string ("backend-id",
                                                          "backend-id",
                                                          "Backend ID",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_CURRENCIES,
                                     g_param_spec_boxed ("currencies",
                                                         "currencies",
                                                         "Currencies this payment method supports",
                                                         G_TYPE_STRV,
                                                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_DESCRIPTION,
                                     g_param_spec_string ("description",
                                                          "description",
                                                          "Description of payment method",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_ID,
                                     g_param_spec_int64 ("id",
                                                         "id",
                                                         "Payment ID",
                                                         G_MININT64, G_MAXINT64, 0,
                                                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_PREFERRED,
                                     g_param_spec_boolean ("preferred",
                                                           "preferred",
                                                           "TRUE if a preferred method",
                                                           FALSE,
                                                           G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_REQUIRES_INTERACTION,
                                     g_param_spec_boolean ("requires-interaction",
                                                           "requires-interaction",
                                                           "TRUE if requires interaction",
                                                           FALSE,
                                                           G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
snapd_payment_method_init (SnapdPaymentMethod *payment_method)
{
}
