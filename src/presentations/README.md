# Custom presentations

`PresentationLibrary` owns named, reusable custom slide sets. It persists validated slide JSON in `presentations.sqlite`, uses bound SQL parameters, and updates an existing entry by stable ID. It does not own editor state or live output. Programs receive their own copies through `ProgramStore.addPresentation`.

The QML custom-slides library opens an entry in the shared Design editor. Template editing uses that same editor with its own context; there is no mode tab inside the editor. SavePresentationDialog handles naming, updating and saving copies.
