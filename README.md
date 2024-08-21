AltTabber
=========

Right, this app is a silly alternative to Alt-Tab that works somewhat better on multiple monitors (if by *better* you understand *differently*). You have to see it for yourself.

This project lives at https://github.com/alzwded/AltTabber/ .

The keys and functionality:

(*hotkey refers to the global hotkey associated with AltTabber. By default, it is `ALT`-`key above tab`*)

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
 which, if set to `0x00000000`, will prevent the AltTabber interface from being reset when you close a window using the context menu.

If you want to remove this app from your computer forever, remember to also delete that key. No uninstaller is currently provided, and it's unlikely that one will be provided anytime in the future.

That key also contains a flag, `resetOnClose`, which, if set to `0x00000000`, will prevent the AltTabber interface from being reset when you close a window using the context menu.

As of version 1.9, AltTabber implements [UI Automation](https://learn.microsoft.com/en-us/windows/win32/winauto/uiauto-uiautomationoverview). The overview presents the thumbnails as *button* control types with the *invoke* pattern. The window title is read when one of these thumbnail buttons comes into view (same text that you see rendered on screen). It is recommended to use AltTabber's builtin keyboard navigation (i.e. `TAB`, `SHIFT`+`TAB`, `ENTER` and `ESCAPE`) since that is the code path that was most thoroughly tested in the past decade. Screen reader commands to the tune of "move to next control" and "activate/invoke" control seem to work, but I am not a proficient AT user, so it's hard to judge if I got the UIA provider implementation right.

Accessibility is not thoroughly tested. If you do test it, and it has problems, do let me know. Though I understand both JAWS and NVDA already have a fancy window list builtin, so I'd also like to know why you are here :-).

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
