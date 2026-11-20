# Browser remote

Offline assets for the optional LAN service: semantic HTML, responsive dark CSS and a dependency-free JavaScript client. The native desktop editor remains Qt Quick.

The client pairs through an HttpOnly session cookie, polls current state, fetches service order only when the live snapshot changes and sends commands only while connected. It never stores credentials or queues offline commands. Insert presentation content with `textContent`, never HTML. Speaker roles are enforced in C++ as well as the UI.

CMake bundles the three assets into the application and test executable. Restart/reload the browser after rebuilding. Use the isolated `--remote-demo` fixture described in the [remote guide](../docs/features/remote-control.md) for integration checks. `node --check remote/app.js` checks syntax without a browser.
