# MicMuteToggle

This software:
- makes F15 a global 'mute microphone' hotkey
- makes F16 a global 'unmute microphone' hotkey
- when the microphone is muted, it makes the mouse's first extra/side button a push-to-talk button
- when the microphone is not muted, it makes the mouse's first extra/side button a push-to-mute button
- when PTT is being used, there is a delay of 250ms after releasing the button before the microphone is re-muted. This is because
  people instincively release the button as soon as they have started prononucing the last syllable, rather than when they have finished

Your keyboard is unlikely to have F15 or F16 buttons; you can likely bind these via your keyboard or mouse customization software, or trigger
them via something like an Elgato StreamDeck.

For a more-flexible (but less compatible) solution, consider [ahk-micmute](https://github.com/fredemmott/ahk-micmute).

MicMuteToggle is useful for programs that do not interoperate well with AutoHotKey; for example, Call Of Duty: Black Ops 4 will immediately exit
on startup if any AutoHotKey script is running (likely releated to cheat detection).

## Usage

1. Download from the releases page
2. Run the program; it can be killed from task manager

If you want it to run on startup:

1. hit Win+R
2. type `shell:startup` and hit enter
3. Create a shortcut to the executable

## Configuration

No end-user configuration is available. The keys, buttons, and delays can be edited by changing the constants at the top of `MicMuteToggle.cpp`,
then rebuilding with Microsoft Visual Studio.

## License

The C++ source code is licensed under the [MIT License](COPYING.SOURCE_CODE).

To the extent possible under law, Frederick Emmott has waived all copyright and related or neighboring rights to the `mute.wav` and `unmute.wav`
files. For details, see [COPYING.AUDIO-FILES](COPYING.AUDIO-FILES).