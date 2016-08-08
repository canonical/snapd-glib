#ifndef __SNAPD_PAYMENT_METHOD_LIST_H__
#define __SNAPD_PAYMENT_METHOD_LIST_H__

#if !defined(__SNAPD_GLIB_INSIDE__) && !defined(SNAPD_COMPILATION)
#error "Only <snapd-glib/snapd-glib.h> can be included directly."
#endif

#include <glib-object.h>

#include <snapd-glib/snapd-payment-method.h>

G_BEGIN_DECLS

#define SNAPD_TYPE_PAYMENT_METHOD_LIST  (snapd_payment_method_list_get_type ())

G_DECLARE_FINAL_TYPE (SnapdPaymentMethodList, snapd_payment_method_list, SNAPD, PAYMENT_METHOD_LIST, GObject)

struct _SnapdPaymentMethodListClass
{
    /*< private >*/
    GObjectClass parent_class;
};

// FIXME: Make private
void               _snapd_payment_method_list_add    (SnapdPaymentMethodList *payment_method_list,
                                                      SnapdPaymentMethod     *snap);

gsize               snapd_payment_method_list_length (SnapdPaymentMethodList *payment_method_list);

SnapdPaymentMethod *snapd_payment_method_list_index  (SnapdPaymentMethodList *payment_method_list,
                                                      gsize                   index);

G_END_DECLS

#endif /* __SNAPD_PAYMENT_METHOD_LIST_H__ */
