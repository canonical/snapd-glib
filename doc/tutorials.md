Title: Tutorials

# Build and Run the Examples

Three examples are provided to demonstrate use of snapd-glib:
1. `snap-glib.c`
1. `snapctl-glib.c`
1. `snapd-glib.py`

## `snap-glib.c`
This example mimics the `snap` utility, and provides implementations of the commands: find, info, install, remove, list, logs, model, help

## `snapctl-glib.c`
This example runs `snapctl` commands via a socket connection into the `snap` sandbox.

## `snapd-glib.py`
This example shows how snapd-glib can be used in Python applications

## Build and Run Examples
The examples are built as part of the main build process, and can be found in the `_build/examples` directory.

### `snap-glib`
``` bash
$ ./snap-glib help
```
```
Usage snap-glib <command> [<options>...]
Commands: find, info, install, remove, list, logs, model, help
```
``` bash
$ cd _build/examples
$ ./snap-glib list
```
```
Name                         Version                         Rev    Tracking                    Publisher
mesa-2404                    25.2.8-snap288                  1839   latest/stable               Canonical
thunderbird                  154.0-2                         1228   latest/stable               Canonical
...
core24                       20260410                        1643   latest/stable               Canonical
```

### `snapctl-glib`
`sudo` might be required.
```bash
$ sudo ./snapctl-glib help
```
```
[sudo: authenticate] Password:       
error: (null): snapctl: Unknown command `help'. Please specify one command of: fde-setup-request, fde-setup-result, get, install, is-connected, kmod, model, mount, reboot, remove, restart, services, set, set-health, start, stop, system-mode, umount, unset or version
```
```bash
$ sudo ./snapctl-glib version
```
```
snapd   2.76.3
series  16
```

### `snapd-glib.py`
```bash
$ python3 snapd-glib.py
```
```
------------------------------------------
Snap ID    : YoNORAsMI6sRqco2IJ6DygSnbxuugTrD
Name       : epiphany
Version    : 48.5
Summary    : Web browser for GNOME
Description: The web browser for GNOME, featuring tight integration with the desktop
and a simple and intuitive user interface that allows you to focus on your
web pages. If you’re looking for a simple, clean, beautiful view of the
web, this is the browser for you.

GNOME Web is often referred to by its code name, Epiphany.
```