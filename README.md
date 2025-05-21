## snapd-glib

snapd-glib is a library to allow GLib based applications access to snapd, the daemon that controls Snaps.
A snapd-qt library is provided that wraps snapd-glib for Qt based applications.
The following languages / platforms are supported:
  * C
  * C++
  * Vala
  * Python (using GObject introspection)
  * Javacript (using GObject introspection)
  * Qt
  * QML

Documentation is available [online](https://canonical.github.io/snapd-glib/). See [snapcraft.io](https://snapcraft.io) for more information on snapd.

## Contributing

If you want to contribute with the project, please, read first the CONTRIBUTING.md file.

When sending a patch, always ensure that the tests do work and that the coding style test
passes. For the latter, the best way is to edit the `.git/hooks/pre-commit` script and set
it to

    #!/bin/sh
    ./format-source.sh check

Also, just manually running `./format-source.sh` script, without parameters, will do an
automatic formatting.

## Reporting bugs

If you have found an issue with the application, please [file an issue](https://github.com/canonical/snapd-glib/issues/new) on the [issues list on GitHub](https://github.com/snapcore/snapd-glib/issues).
