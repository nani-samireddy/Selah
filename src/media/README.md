# Media catalog

`MediaLibrary` catalogs managed images, videos and audio independently of slides, discovers older managed imports and schedules thumbnails. Video frames come from the owning QMediaPlayer/QVideoSink.

Removing a slide layer must not delete a shared asset. Do not decode every video for thumbnails simultaneously or block the GUI while copying large media.

See [architecture](../../docs/architecture.md), [coding standards](../../docs/coding-standards.md) and the corresponding model/smoke cases under [tests](../../tests/README.md).

Audio assets are cataloged without thumbnail decoding. Imports are copied into the managed media folder and retain `kind: audio` across editor/program persistence, recovery and bundles. `LiveAudio` owns one decoder per live audio element independently of audience/NDI renderer instances. `AudienceRenderer` disables embedded audio decoding; explicit editor/media previews use their own muted-by-default playback. New audio layers default to no loop and no per-layer mute, while live output still obeys the global audio switch.
