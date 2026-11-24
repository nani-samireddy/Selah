# Offline guide content

`GuideContent` is a QML singleton in the ChurchDeck module. It reads a fixed list
of Markdown topics from the application's `/guides` resources. It accepts topic
filenames, not filesystem paths, and requires no network or QML file-access flag.

Edit articles in `docs/user-guide/`. CMake bundles those same files for the reader.
Walkthrough behavior and topic metadata live in `qml/help/`.
