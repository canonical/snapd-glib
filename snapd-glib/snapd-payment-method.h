#ifndef __SNAPD_PAYMENT_METHOD_H__
#define __SNAPD_PAYMENT_METHOD_H__

#if !defined(__SNAPD_GLIB_INSIDE__) && !defined(SNAPD_COMPILATION)
#error "Only <snapd-glib/snapd-glib.h> can be included directly."
#endif

#include <glib-object.h>

G_BEGIN_DECLS

#define SNAPD_TYPE_PAYMENT_METHOD  (snapd_payment_method_get_type ())

G_DECLARE_FINAL_TYPE (SnapdPaymentMethod, snapd_payment_method, SNAPD, PAYMENT_METHOD, GObject)

struct _SnapdPaymentMethodClass
{
    /*< private >*/
    GObjectClass parent_class;
};

gchar     *snapd_payment_method_get_backend_id           (SnapdPaymentMethod *payment_method);

gchar    **snapd_payment_method_get_currencies           (SnapdPaymentMethod *payment_method);

gchar     *snapd_payment_method_get_description          (SnapdPaymentMethod *payment_method);

gint64     snapd_payment_method_get_id                   (SnapdPaymentMethod *payment_method);

gboolean   snapd_payment_method_get_preferred            (SnapdPaymentMethod *payment_method);

gboolean   snapd_payment_method_get_requires_interaction (SnapdPaymentMethod *payment_method);

G_END_DECLS

#endif /* __SNAPD_PAYMENT_METHOD_H__ */
