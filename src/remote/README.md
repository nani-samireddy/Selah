# Remote runtime

`RemoteController` is explicitly started by the desktop and bound to one offered private IPv4 address (or loopback for testing). It owns memory-only sessions and pairing codes. `RemoteApi.cpp` implements fixed routes and role-enforced commands against `OutputManager`'s copied live snapshot. No program/editor draft references are accepted from remote clients.

`RemoteHttpServer` is a bounded HTTP/1.1 transport, not a general-purpose static file server. `RemotePreview` renders small JPEG stills and uses the pinned QR encoder. Browser assets are bundled from the root `remote/` directory by CMake.

Keep access checks on the server, all user content plain text, and remote credentials out of persisted settings/logs. Preserve stale-snapshot/index checks and command deduplication. See the [remote guide](../../docs/features/remote-control.md) for protocol constraints, testing and HTTP/trusted-network limitations.
