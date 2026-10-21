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
