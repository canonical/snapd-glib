/*
 * Copyright (C) 2016 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include "snapd-price.h"

/**
 * SECTION: snapd-price
 * @short_description: Pricing information
 * @include: snapd-glib/snapd-glib.h
 *
 * A #SnapdPrice represents an amount of money required to purchase a snap.
 * Prices can be queried using snapd_snap_get_prices() and are used in
 * snapd_client_buy_sync().
 */

/**
 * SnapdPrice:
 *
 * #SnapdPrice contains pricing information.
 *
 * Since: 1.0
 */

struct _SnapdPrice
{
    GObject parent_instance;

    gdouble amount;
    gchar *currency;
};

enum
{
    PROP_AMOUNT = 1,
    PROP_CURRENCY,
    PROP_LAST
};

G_DEFINE_TYPE (SnapdPrice, snapd_price, G_TYPE_OBJECT)

/**
 * snapd_price_get_amount:
 * @self: a #SnapdPrice.
 *
 * Get the currency amount for this price, e.g. 0.99.
 *
 * Return: a currency amount.
 *
 * Since: 1.0
 */
gdouble
snapd_price_get_amount (SnapdPrice *self)
{
    g_return_val_if_fail (SNAPD_IS_PRICE (self), 0.0);
    return self->amount;
}

/**
 * snapd_price_get_currency:
 * @self: a #SnapdPrice.
 *
 * Get the currency this price is in, e.g. "NZD".
 *
 * Returns: an ISO 4217 currency code.
 *
 * Since: 1.0
 */
const gchar *
snapd_price_get_currency (SnapdPrice *self)
{
    g_return_val_if_fail (SNAPD_IS_PRICE (self), NULL);
    return self->currency;
}

static void
snapd_price_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    SnapdPrice *self = SNAPD_PRICE (object);

    switch (prop_id) {
    case PROP_AMOUNT:
        self->amount = g_value_get_double (value);
        break;
    case PROP_CURRENCY:
        g_free (self->currency);
        self->currency = g_strdup (g_value_get_string (value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
snapd_price_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    SnapdPrice *self = SNAPD_PRICE (object);

    switch (prop_id) {
    case PROP_AMOUNT:
        g_value_set_double (value, self->amount);
        break;
    case PROP_CURRENCY:
        g_value_set_string (value, self->currency);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
snapd_price_finalize (GObject *object)
{
    SnapdPrice *self = SNAPD_PRICE (object);

    g_clear_pointer (&self->currency, g_free);

    G_OBJECT_CLASS (snapd_price_parent_class)->finalize (object);
}

static void
snapd_price_class_init (SnapdPriceClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->set_property = snapd_price_set_property;
    gobject_class->get_property = snapd_price_get_property;
    gobject_class->finalize = snapd_price_finalize;

    g_object_class_install_property (gobject_class,
                                     PROP_AMOUNT,
                                     g_param_spec_double ("amount",
                                                          "amount",
                                                          "Amount of price",
                                                          0.0, G_MAXDOUBLE, 0.0,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property (gobject_class,
                                     PROP_CURRENCY,
                                     g_param_spec_string ("currency",
                                                          "currency",
                                                          "Currency amount is in",
                                                          NULL,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
snapd_price_init (SnapdPrice *self)
{
}
