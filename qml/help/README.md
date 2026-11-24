# Learning Selah

- `GuideTopics.js` defines searchable topic metadata and walkthrough steps.
- `UserGuideDialog.qml` reads bundled Markdown through the `GuideContent` singleton
  and starts or replays walkthroughs.
- `UserGuideTour.qml` routes between workspaces, highlights live controls by
  object name, and presents nonmodal Previous/Next coaching. Completion is a local
  preference. Ending a tour keeps the current workspace and the user's edits.

Steps may reveal a workspace or library, but must never create content, load a
program, start a remote server, enable displays or change live output. Those
operations remain explicit user actions. Keep target object names stable and
update the workspace smoke checks when adding a route.
