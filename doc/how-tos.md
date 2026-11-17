Title: How to ...

# Build and Install `snapd-glib`
Build and installation uses `meson` and `ninja` build system. Documentation covering installation and use is available at:
- [`meson`](https://mesonbuild.com/index.html)

Build steps:
1. Check which options in `meson_options.txt` you want to override
1. Run the commands below to perform the build in a directory named `_build`
```
$ meson setup _build -Ddocs=true -Dqt5=false -Dqt6=false
$ meson compile -C _build
```
To install the built artifacts, run:
```
$ meson install -C _build
```

# Include snapd-glib in an application

## C Applications
``` C
#include <snapd-glib/snapd-glib.h>
...
```
See `examples/snap-glib.c` and `examples/snapctl-glib.c` for detailed use.

## Python
``` python
import gi

# Specify the version required by the library (check your library's documentation)
gi.require_version('Snapd', '2')

from gi.repository import Snapd

# Example using Snapd from Python
client = Snapd.Client.new()
snaps = Snapd.Client.find_sync(client, 0, 'epiphany', None)
for snap in snaps[0]:
    print('------------------------------------------')
    print('Snap ID    : ' + snap.get_id())
    print('Name       : ' + snap.get_name())
    print('Version    : ' + snap.get_version())
    print('Summary    : ' + snap.get_summary())
    print('Description: ' + snap.get_description())
```

# Use Authentication
Authentication requires an Ubuntu account, which can be set up at [Ubuntu](https://login.ubuntu.com) - it is not directly related to your operating system account.

## C
```c
g_autoptr(SnapdUserInformation) user =
snapd_client_login2_sync(client, email, password, otp, NULL, &error);

if (user != NULL) {
    SnapdAuthData *auth_data =
        snapd_user_information_get_auth_data(user);
    snapd_client_set_auth_data(client, auth_data);
}
```

`SnapdAuthData` contains the serialized macaroon and discharge data used in subsequent requests. It is designed to be stored securely between sessions so that the user does not need to authenticate repeatedly. Treat this data like a credential.

The older `snapd_client_login_*()` and standalone `snapd_login_*()` APIs are deprecated. New code should use the `login2` methods and `SnapdUserInformation`.

## Python
``` python
import gi

# Specify the version required by the library (check your library's documentation)
gi.require_version('Snapd', '2')

from gi.repository import Snapd

# Example using Snapd from Python
client = Snapd.Client.new()

user = Snapd.Client.login2_sync(client, 'my_email', 'my_ubuntu_password', None, None)
if user is not None:
    client.set_auth_data(user.get_auth_data())
```
