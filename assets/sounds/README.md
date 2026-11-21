# Interface sounds

`click.wav` is the user-supplied `click-004.mp3`, decoded to mono 44.1 kHz, 16-bit PCM for Qt SoundEffect. It replaces the synthesized click for button and selection feedback. The source attachment was `click-004.mp3`; playback uses the bundled WAV and does not depend on the Downloads folder.

`select.wav` is the earlier synthesized cue retained as an unused source asset. Both active SoundEffect instances use `click.wav`.

`InteractionFeedback.qml` preloads these assets, limits playback rate and keeps volume low. They belong to the operator interface, separate from media playback and output capture. Sound preferences and automatic muting in Present continue to apply.
