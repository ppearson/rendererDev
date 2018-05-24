
Need to set $KATANA_RENDER_INTERCEPT = /path/to/this_binary


To use custom commands before renderboot itself, set:
$RPB_EXTRA_COMMANDS

to either a single command, or a string of multiple commands separated by '|' chars (in quotes, otherwise shell will expand them) for arguments.

The actual command must be a full path to the executable to run - i.e. it can't just be 'valgrind' or 'heaptrack',
it needs to be /usr/bin/valgrind



