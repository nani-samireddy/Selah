# Interface sound

`click.wav` is the user-provided `click-004.mp3`, decoded to mono 44.1 kHz,
16-bit PCM for Qt SoundEffect without changing its tone. Playback uses the
bundled WAV and does not depend on the source file or Downloads folder.

Button and selection feedback use this sound at playback volumes of 0.18 and
0.15 respectively.

`InteractionFeedback.qml` preloads the sound, limits playback rate, respects the
sound preference, and automatically mutes operator sounds in Present. Interface
feedback remains separate from slide/media audio and output capture.
