Title: Explanations

## Overview

snapd-glib is a GLib and GObject library for applications that need to communicate with [snapd](https://snapcraft.io/docs/snapd). snapd is the daemon responsible for installing, removing, refreshing, and inspecting snaps.

The library provides a typed interface to snapd's REST API. Applications normally work with `SnapdClient`, call an operation, and receive GObject-based model objects such as `SnapdSnap`, `SnapdChange`, or `SnapdSystemInformation`.

The same core library can be used from C, C++, Vala, Python, JavaScript, and other languages supported by GObject introspection. The `snapd-qt` library exposes the same functionality through Qt request and wrapper classes.

## The main components

### `SnapdClient`

`SnapdClient` is the entry point for communication with snapd:

```c
#include <snapd-glib/snapd-glib.h>

SnapdClient *client = snapd_client_new();
```

A client connects on demand to snapd's Unix socket. The default socket is normally `/run/snapd.socket`; applications can select another socket with `snapd_client_set_socket_path()`.

The client stores connection-related state, including the socket path, user agent, authentication data, and maintenance information reported by snapd. A client can issue multiple requests and manages the connections needed to run them.

The old `snapd_client_connect_sync()` and `snapd_client_connect_async()` functions are retained for compatibility, but connecting explicitly is no longer required.

### Requests

Each public client operation maps to a request against snapd. Examples include:

- `snapd_client_get_snaps_*()` for installed snaps
- `snapd_client_find_*()` for store searches
- `snapd_client_install2_*()` and `snapd_client_remove2_*()` for changes
- `snapd_client_get_changes_*()` for tracking changes
- `snapd_client_get_system_information_*()` for device information
- `snapd_client_get_logs_*()` and `snapd_client_get_notices_*()` for runtime events

Internally, requests create an HTTP-style message for snapd's Unix-socket API, send it, parse the JSON or assertion response, and convert it into library objects. Most of this request machinery is private; applications use the public `SnapdClient` methods.

`snapd` provides an HTTP API via the socket. The URL structure is of the form `http://snapd/v2/<type>[?<query>]`. `snapd-glib` uses `libsoup` to implement the HTTP requests.

### Model objects

Responses are represented by immutable-style GObject model objects with getter functions. Common objects include:

- `SnapdSnap` for snap metadata, channels, applications, and installation state
- `SnapdApp` for applications and services provided by a snap
- `SnapdChange` and `SnapdTask` for operations and their progress
- `SnapdSystemInformation` for device and snapd state
- `SnapdConnection`, `SnapdPlug`, and `SnapdSlot` for interface connections
- `SnapdNotice` and `SnapdLog` for events and log entries

Returned containers are usually `GPtrArray` instances. The API documentation describes ownership for each return value; use GLib's automatic cleanup helpers where available, for example `g_autoptr()` and `g_autoptr(GPtrArray)`.

## Synchronous and asynchronous APIs

Most operations have three forms:

1. A `_sync()` function that waits and returns its result.
2. An `_async()` function that starts the operation and invokes a `GAsyncReadyCallback`.
3. A `_finish()` function that extracts the result in the callback.

For example, a synchronous query can be written as:

```c
g_autoptr(SnapdClient) client = snapd_client_new();
g_autoptr(GError) error = NULL;
g_autoptr(GPtrArray) snaps =
    snapd_client_get_snaps_sync(client, SNAPD_GET_SNAPS_FLAGS_NONE,
                                NULL, NULL, &error);

if (snaps == NULL) {
    g_printerr("Failed to get snaps: %s\n", error->message);
    return EXIT_FAILURE;
}

for (guint i = 0; i < snaps->len; i++) {
    SnapdSnap *snap = g_ptr_array_index(snaps, i);
    g_print("%s\n", snapd_snap_get_name(snap));
}
```

The asynchronous form keeps the application responsive while snapd processes the request:

```c
static void
get_snaps_cb(GObject *source_object, GAsyncResult *result,
             gpointer user_data)
{
    SnapdClient *client = SNAPD_CLIENT(source_object);
    g_autoptr(GError) error = NULL;
    g_autoptr(GPtrArray) snaps =
        snapd_client_get_snaps_finish(client, result, &error);

    if (snaps == NULL) {
        g_printerr("Failed to get snaps: %s\n", error->message);
        return;
    }

    /* Use the returned SnapdSnap objects here. */
}

snapd_client_get_snaps_async(client, SNAPD_GET_SNAPS_FLAGS_NONE,
                             NULL, NULL, get_snaps_cb, user_data);
```

The synchronous wrappers are implemented on top of the asynchronous request machinery using a temporary GLib main context. Use asynchronous methods in event-driven applications, especially when running on the main loop.

A `GCancellable` can be passed to most operations. Cancelling it causes the operation to finish with a cancellation error, allowing an application to stop work when a window, job, or request is no longer needed.

## Long-running changes

Installing, removing, refreshing, and similar operations are asynchronous in snapd. The initial request returns a change identifier. snapd-glib follows that change and reports progress through the operation's progress callback or the resulting `SnapdChange` object.

A `SnapdChange` contains the change ID, kind, summary, status, readiness, error text, timestamps, and a list of `SnapdTask` objects. Tasks describe the individual work being performed and can include progress counters.

Applications should inspect the change status and tasks rather than assuming that receiving the initial response means the operation has completed. The client also provides methods to list changes and retrieve a specific change by ID.

## Authentication

Operations that modify the system or access private store data may require authorization. Applications authenticate with `snapd_client_login2_sync()` or its asynchronous pair:

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

## Errors and maintenance state

Methods report failures through `GError`. snapd-glib uses the `SNAPD_ERROR` domain for errors such as connection failures, invalid authentication, permission errors, unavailable snaps, and change conflicts. Always check the error before using a returned object or container.

For example:

```c
if (!snapd_client_remove2_sync(client, SNAPD_REMOVE_FLAGS_NONE, name,
                               NULL, NULL, NULL, &error)) {
    if (g_error_matches(error, SNAPD_ERROR, SNAPD_ERROR_NOT_INSTALLED))
        g_print("The snap is not installed\n");
    else
        g_printerr("Remove failed: %s\n", error->message);
}
```

snapd can also report maintenance information while handling a request. After an operation, applications can inspect `snapd_client_get_maintenance()` and decide whether to show that information to the user.

## Events and monitoring

For one-time reads, use the relevant `get_*` method. For ongoing activity, use a follow or monitor API:

- `snapd_client_follow_logs_sync()` follows logs and invokes a `SnapdLogCallback` for each entry.
- `SnapdNoticesMonitor` waits for new snapd notices and emits `notice-event` signals.
- Change objects can be queried repeatedly when an application needs to display operation progress.

Monitors own their cancellable state and should be stopped before they are disposed. As with other asynchronous APIs, handle errors and cancellation in the callback or signal handler.

## Typical application flow

A GLib application commonly follows this sequence:

1. Create one `SnapdClient` and configure its socket path or user agent if necessary.
2. Use asynchronous client methods from the application's main loop.
3. Read typed model objects from the result in the finish callback.
4. Keep and poll or follow `SnapdChange` objects for long-running operations.
5. Store authentication data securely if login is required.
6. Pass a `GCancellable` to work that may outlive the UI or calling task.
7. Unref objects and containers according to the ownership annotations in the API documentation.

The `examples/` directory contains complete small programs using these patterns, while the generated API reference documents every client operation and model type.

## Building applications

Applications using the C API need GLib, GIO, JSON-GLib, libsoup, and the snapd-glib development package. A typical compiler command uses `pkg-config` to obtain the required flags:

```sh
cc $(pkg-config --cflags snapd-glib) -o my-snap-tool my-snap-tool.c \
   $(pkg-config --libs snapd-glib)
```

Include the umbrella header, `<snapd-glib/snapd-glib.h>`, rather than including individual public headers directly. The generated API reference is the best place to check availability annotations, ownership, deprecations, and the corresponding `_async()`/`_finish()` pairs.
