/*
 * Copyright (C) 2016 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include "config.h"

#include "snapd-price.h"

struct _SnapdPrice
{
    GObject parent_instance;

    gchar *amount;
    gchar *currency;
};

enum 
{
    PROP_AMOUNT = 1,
    PROP_CURRENCY,  
    PROP_LAST
};
 
G_DEFINE_TYPE (SnapdPrice, snapd_price, G_TYPE_OBJECT)

SnapdPrice *
snapd_price_new (void)
{
    return g_object_new (SNAPD_TYPE_PRICE, NULL);
}

const gchar *
snapd_price_get_amount (SnapdPrice *price)
{
    g_return_val_if_fail (SNAPD_IS_PRICE (price), NULL);
    return price->amount;
}

const gchar *
snapd_price_get_currency (SnapdPrice *price)
{
    g_return_val_if_fail (SNAPD_IS_PRICE (price), NULL);
    return price->currency;
}

static void
snapd_price_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    SnapdPrice *price = SNAPD_PRICE (object);

    switch (prop_id) {
    case PROP_AMOUNT:
        g_free (price->amount);
        price->amount = g_strdup (g_value_get_string (value));
        break;
    case PROP_CURRENCY:
        g_free (price->currency);
        price->currency = g_strdup (g_value_get_string (value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
snapd_price_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    SnapdPrice *price = SNAPD_PRICE (object);

    switch (prop_id) {
    case PROP_AMOUNT:
        g_value_set_string (value, price->amount);
        break;
    case PROP_CURRENCY:
        g_value_set_string (value, price->currency);
        break;      
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
snapd_price_finalize (GObject *object)
{
    SnapdPrice *price = SNAPD_PRICE (object);

    g_clear_pointer (&price->amount, g_free);
    g_clear_pointer (&price->currency, g_free);
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
                                     g_param_spec_string ("amount",
                                                          "amount",
                                                          "Amount of price",
                                                          NULL,
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
snapd_price_init (SnapdPrice *price)
{
}
