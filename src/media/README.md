# Media catalog

`MediaLibrary` catalogs managed images/videos independently of slides, discovers older managed imports and schedules thumbnails. Video frames come from the owning QMediaPlayer/QVideoSink.

Removing a slide layer must not delete a shared asset. Do not decode every video for thumbnails simultaneously or block the GUI while copying large media.

See [architecture](../../docs/architecture.md), [coding standards](../../docs/coding-standards.md) and the corresponding model/smoke cases under [tests](../../tests/README.md).
