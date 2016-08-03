#ifndef __SNAPD_PRICE_H__
#define __SNAPD_PRICE_H__

#if !defined(__SNAPD_GLIB_INSIDE__) && !defined(SNAPD_COMPILATION)
#error "Only <snapd-glib/snapd-glib.h> can be included directly."
#endif

#include <glib-object.h>

G_BEGIN_DECLS

#define SNAPD_TYPE_PRICE  (snapd_price_get_type ())

G_DECLARE_FINAL_TYPE (SnapdPrice, snapd_price, SNAPD, PRICE, GObject)

struct _SnapdPriceClass
{
    /*< private >*/
    GObjectClass parent_class;
};

const gchar *snapd_price_get_amount   (SnapdPrice *price);

const gchar *snapd_price_get_currency (SnapdPrice *price);

G_END_DECLS

#endif /* __SNAPD_PRICE_H__ */
