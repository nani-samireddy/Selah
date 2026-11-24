# Interface sounds

The bundled cues come from [Kenney Interface Sounds 1.0](https://kenney.nl/assets/interface-sounds), licensed CC0. The source mirror is [Calinou/kenney-interface-sounds](https://github.com/Calinou/kenney-interface-sounds). The original license is included as `Kenney-LICENSE.txt`.

| Bundled file | Original file | Use |
| --- | --- | --- |
| click.wav | click_002.wav | Button activation |
| select.wav | select_001.wav | Selection |
| toggle.wav | toggle_002.wav | Checkable controls |
| switch.wav | switch_001.wav | Expand panels |
| back.wav | back_001.wav | Collapse panels |
| drop.wav | drop_001.wav | Layer reorder |
| pluck.wav | pluck_001.wav | Success cue available to callers |
| tick.wav | tick_001.wav | Fine feedback cue available to callers |

Cues are mono 44.1 kHz PCM, filtered to 90–2200 Hz, with an 8 ms attack and 25 ms release fade. Peak levels are normalized to -22 dBFS before playback gain. This set replaces the previous user-supplied click.

`InteractionFeedback.qml` preloads the cues, limits playback rate, and respects the sound preference. Presentation mode automatically mutes operator sounds. They are separate from slide/media audio and output capture.
