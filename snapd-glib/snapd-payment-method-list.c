#include "config.h"

#include "snapd-payment-method-list.h"

struct _SnapdPaymentMethodList
{
    GObject parent_instance;

    GPtrArray *payment_methods;
};

G_DEFINE_TYPE (SnapdPaymentMethodList, snapd_payment_method_list, G_TYPE_OBJECT)

void
_snapd_payment_method_list_add (SnapdPaymentMethodList *payment_method_list, SnapdPaymentMethod *snap)
{
    g_ptr_array_add (payment_method_list->payment_methods, g_object_ref (snap));
}

gsize
snapd_payment_method_list_length (SnapdPaymentMethodList *payment_method_list)
{
    g_return_val_if_fail (SNAPD_IS_PAYMENT_METHOD_LIST (payment_method_list), 0);
    return payment_method_list->payment_methods->len;
}

SnapdPaymentMethod *
snapd_payment_method_list_index (SnapdPaymentMethodList *payment_method_list, gsize index)
{
    g_return_val_if_fail (SNAPD_IS_PAYMENT_METHOD_LIST (payment_method_list), NULL);  
    return payment_method_list->payment_methods->pdata[index];
}

static void
snapd_payment_method_list_finalize (GObject *object)
{
    SnapdPaymentMethodList *payment_method_list = SNAPD_PAYMENT_METHOD_LIST (object);

    g_ptr_array_foreach (payment_method_list->payment_methods, (GFunc) g_object_unref, FALSE);
    g_ptr_array_free (payment_method_list->payment_methods, TRUE);
    payment_method_list->payment_methods = NULL;
}

static void
snapd_payment_method_list_class_init (SnapdPaymentMethodListClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->finalize = snapd_payment_method_list_finalize;
}

static void
snapd_payment_method_list_init (SnapdPaymentMethodList *payment_method_list)
{
    payment_method_list->payment_methods = g_ptr_array_new ();
}
