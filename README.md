AltTabber
=========

Right, this app is a silly alternative to Alt-Tab that works somewhat better on multiple monitors (if by *better* you understand *differently*). You have to see it for yourself.

The keys and functionality:

| action | inputs | comments |
|-----------|-----------|-----------------|
| open overlay | `Ctrl`-`Alt`-`3` or clicking the icon in the notification area | the main bread of the application |
| close overlay | `Esc` | exits the overlay without switching to a new window and without exiting |
| exit the application | `Alt`-`F4` | duh |
| move to other windows | `Tab`, `Shift`-`Tab`, arrow keys, mouse + click, mouse wheel | this doesn't actually switch the window, rather the selection for actually triggering the selection |
| switch window | `Enter`, double click, `Ctrl`-`Alt`-`3` | switches to the selected window (the one with a red background |
| help/about | `F1` | |
| start/stop logging | `F2` | this is a debug log; it writes it somewhere in `%TEMP%` in a  file with a name like `alt????.tmp` |
| search | any printable characters | starts filtering the windows by name and/or image path in order to help find your window |
