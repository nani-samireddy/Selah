# Broadcast prototype

`NdiController` owns GUI-thread offscreen rendering/readback. `NdiWorker` consumes straight-alpha frames through a bounded queue and optionally loads the official NDI runtime.

Preserve the GUI/render-thread boundary and frame-buffer lifetime. Stop the sender before releasing render resources. Qt GuiPrivate APIs require validation on Qt upgrades; network interoperability and NDI audio are not yet verified.

See [architecture](../../docs/architecture.md), [coding standards](../../docs/coding-standards.md) and the corresponding model/smoke cases under [tests](../../tests/README.md).
