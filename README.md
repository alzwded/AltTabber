AltTabber
=========

AltTabber started back in the Windows 7 days out of frustration with managing a large number of windows. There were no good virtual desktop implementations, and the taskbar was getting ever more annoying compared to ancient versions of Windows. Additionally, I started having more and more monitors connected, and it was getting a bit out of hand to quickly identify "the Visual Studio that's on the 3rd monitor".

The goals of this project were to have a hotkey activation, similar to `ALT` + `TAB` or Gnome's `ALT` + `F1` / `super`, which would lay out a grid of all windows present on a monitor. The grid(s) may be filtered with a simple search string. You may activate windows primarily through keyboard use, but mouse is supported. It shall show *DWM* live thumbnails if Aero is running. It shall be lightweight.

This was successful, but Windows has made things less of a problem as we went from 7 through 8.1 to 11. But I still use it purely because of AltTabber's keyboard-first design, and especially because I can very quickly type *n o t e p a d* and list all 20 open notepad windows, and I can then figure out where I wanted to go.

This project lives at https://github.com/alzwded/AltTabber/ .

Key bindings and activation
---------------------------

(*hotkey refers to the global hotkey associated with AltTabber. By default, it is `ALT` + `key above tab`*)

| action | inputs | comments |
|-----------|-----------|-----------------|
| open overlay | *hotkey* or clicking the icon in the notification area | the main bread of the application |
| close overlay | `Esc` | exits the overlay without switching to a new window and without exiting |
| exit the application | `Alt`-`F4` | duh |
| move to other windows | `Tab`, `Shift`-`Tab`, arrow keys, mouse + click, mouse wheel | this doesn't actually switch the window, rather the selection for actually triggering the selection |
| switch window | `Enter`, double click, *hotkey* | switches to the selected window (the one with a red background |
| help/about | `F1` | |
| start/stop logging | `F2` | this is a debug log; it writes it somewhere in `%TEMP%` in a  file with a name like `alt????.tmp` |
| search/filter | any printable characters | starts filtering the windows by name and/or image path in order to help find your window among a couple of dozen |

Changing the hotkey
-------------------

The hotkey can be changed from `Alt`-`key above tab` by going in the registry under `HKEY_CURRENT_USER\Software\jakkal\AltTabber` and modifying the `modifiers` and `key` values.

The `modifiers` value is a combination of [the values described for fsModifiers here](http://msdn.microsoft.com/en-us/library/windows/desktop/ms646309.aspx) things.

The `key` value is one of [these](http://msdn.microsoft.com/en-us/library/windows/desktop/dd375731.aspx).

For example, you can set the `modifiers` value to `3` (Alt + Ctrl) and the `key` value to `33` (the `3` key). If you didn't catch on, the aforementioned values are numbers in hexadecimal.

If you mess something up, just delete the `HKEY_CURRENT_USER\Software\jakka\AltTabber` key and the app will make sure to recreate it again from defaults.

If you want to remove this app from your computer forever, remember to also delete that key. No uninstaller is currently provided, and it's unlikely that one will be provided anytime in the future.

That key also contains a flag, `resetOnClose`, which, if set to `0x00000000`, will prevent the AltTabber interface from being reset when you close a window using the context menu. This leaves some dangling slots/thumbnails around, but the resets were getting annoying.

Accessibility
-------------

As of version 1.9, AltTabber implements [UI Automation](https://learn.microsoft.com/en-us/windows/win32/winauto/uiauto-uiautomationoverview). The overview presents the thumbnails as *button* control types with the *invoke* pattern. The window title is read when one of these thumbnail buttons comes into view (same text that you see rendered on screen). It is recommended to use AltTabber's builtin keyboard navigation (i.e. `TAB`, `SHIFT`+`TAB`, `ENTER` and `ESCAPE`) since that is the code path that was most thoroughly tested in the past decade. Screen reader commands to the tune of "move to next control" and "activate/invoke" control seem to work, but I am not a proficient AT user, so it's hard to judge if I got the UIA provider implementation right.

Accessibility is not thoroughly tested. If you do test it, and it has problems, do let me know. Though I understand both JAWS and NVDA already have a fancy window list builtin, so I'd also like to know why you are here :-).

Hacking
-------

This project was a hackjob to begin with, but here is an outline:

- `AltTabber.cpp` holds the `WinMain`, message loops, and other plumbing typical of a Windows application. Global variables are declared here.
  * it also contains the complicated code of figuring out which windows need to be shown, what their titles are, and fetching their thumbnails
  * `MoveToMonitor`: I have had attempts to deal with minimized windows, e.g. to move them from one desktop to another, but many, many programs don't like that. Moving maximized windows around also lead to weirdness.
  * The *move to monitor n* context menu entries are capped at 10. I hope you don't have more than 10 monitors on your workstation. If you do, let me know how you did it!
  * A global hotkey is installed, per what's specified in the registry (see below)
- Program state is stored in `ProgramState_t`, of which there should only ever be one.
- The notification icon exists solely to let a user know this thing is running at all. There is no attempt to restore it e.g. after restarting the `explorer.exe` instance that runs as the shell. And yes, I am aware this usage is universally despised by UX people, but I like to know when something is running without bringing up `taskmgr.exe`
- Persistent state is stored in the registry under `HKCU\Software\jakkal\AltTabber`. This is hardcoded. If the key or values are not there, they are initialized from defaults. See `Registry.cpp`
- `Gui.cpp` basically contains the window `OnPaint` function, but also additional layouting to position DWM Thumbnails
  * it still has the *icons* code path, which is a holdover from Windows 7, where *Aero* might not have been running. This is no longer applicable on Windows 8+
  * I opted to have a bit OnPaint function which renders the grid, because constantly creating/destroying e.g. MFC controls can get stupid slow and complicate things. We just need to lay out a grid of labels with some space set aside for live thumbnails, it's not that hard to write/maintain
- `Log.cpp` has the log implementation. Normally, logs go `>NUL`. Logging must be turned on with the `F2` key, which will write the file out to a tempfile. It force flushes after each print, to support re-living the final moments before it goes bang.
- `MoveFunctions.cpp` contains spaghetti logic dealing with keyboard arrow or mouse movement between grid cells. This makes stuff like using the arrow keys to go cross monitors work.
- `MonitorGeom.cpp` contains code to figure out where all your monitors are. Windows is a bit like X11, and there is a single giant canvas, with monitors being views on this canvas. Unlike X11, I don't think monitors may be positioned arbitrarily, at least not to my knowledge. I have only tested with monitors whose bounds touch :-) But I have extensively used this with monitors with mismatched resolutions, arranged both vertically and horizontally
- `OverlayActivation.cpp` contains the code to turn the overlay, and when deactivating, to switch focus to the window you selected. Thumbnails and slots are refreshed when activating the overlay, or when filtering
- A11Y is implemented in `UIAutomationProvider.cpp`, using the *UI Automation* interfaces; but:
  * the global `ProgramState_t` holds a reference to the root provider, if any, in order to ping it when state changed
  * as the main message loop is in `AltTabber.cpp`, it is created there (upon `WM_GETOBJECT` being received)
  * There is an atomic int called `rebuildingSlots` which is used as a mutex (which is not the same thing) in order to prevent access to the slots while they are rebuilding, because *UI Automation* injects threads in the application. This should be a Mutex, but that is heavy handed when UIA is not running. This appears good enough as I couldn't reproduce seg faults anymore on either high end or low end hardware; if you can, switch this to a mutex.


A large amount of windows need to be filtered out. You'd be surprised how many eye candy rich apps have a ton of floating BS to make them look pretty. Windows 8 added *cloaking* to have an intended way to do this. *Cloaked* windows participate in DWM rendering, but they are meant to be ignored for all intents and purposes, i.e. all the BS that older applications used to do for eye candy effects.

Accessibility isn't terribly well tested, but screen readers can "see" it, and navigation works. If you add new "features", make sure they are accessible. Narrator is installed on all Windows machines, turn it on with `WIN` + `CTRL` + `ENTER`, you don't really need to do anything special to check it reads out everything that's going on.

Disclaimer
==========

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
