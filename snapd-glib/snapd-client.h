#ifndef __SNAPD_CLIENT_H__
#define __SNAPD_CLIENT_H__

#if !defined(__SNAPD_GLIB_INSIDE__) && !defined(SNAPD_COMPILATION)
#error "Only <snapd-glib/snapd-glib.h> can be included directly."
#endif

#include <glib-object.h>

G_BEGIN_DECLS

#define SNAPD_TYPE_CLIENT             (snapd_client_get_type ())
#define SNAPD_CLIENT(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), SNAPD_TYPE_CLIENT, SnapdClient))
#define SNAPD_IS_CLIENT(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SNAPD_TYPE_CLIENT))
#define SNAPD_CLIENT_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), SNAPD_TYPE_CLIENT, SnapdClientClass))
#define SNAPD_IS_CLIENT_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), SNAPD_TYPE_CLIENT))
#define SNAPD_CLIENT_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), SNAPD_TYPE_CLIENT, SnapdClientClass))

typedef struct _SnapdClient           SnapdClient;
typedef struct _SnapdClientClass      SnapdClientClass;

struct _SnapdClient
{
  /*< private >*/
  GObject parent_instance;
};

struct _SnapdClientClass
{
  /*< private >*/
  GObjectClass parent_class;

  /* padding, for future expansion */
  void (* _snapd_reserved1) (void);
  void (* _snapd_reserved2) (void);
  void (* _snapd_reserved3) (void);
  void (* _snapd_reserved4) (void);
};

GType snapd_client_get_type (void) G_GNUC_CONST;

SnapdClient *snapd_client_new (void);

void snapd_client_connect_sync (SnapdClient *client);

G_END_DECLS

#endif /* __SNAPD_CLIENT_H__ */
