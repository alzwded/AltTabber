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

Changing the hotkey
-------------------

The hotkey can be changed from `Ctrl`-`Alt`-`3` by going in the registry under `HKEY_CURRENT_USER\Software\jakkal\AltTabber` and modifying the `modifiers` and `key` values.

The `modifiers` value is a combination of [the values described for fsModifiers here](http://msdn.microsoft.com/en-us/library/windows/desktop/ms646309.aspx) things.

The `key` value is one of [these](http://msdn.microsoft.com/en-us/library/windows/desktop/dd375731.aspx).

If you mess something up, just delete the `HKEY_CURRENT_USER\Software\jakka\AltTabber` key and the app will make sure to recreate it again from defaults.

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
