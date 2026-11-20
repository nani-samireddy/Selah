# Tests

`unit/` contains model-test definitions and the standalone runner. `smoke/` contains optional GUI/rendering scenarios and their command router. Runtime sources do not contain test bodies.

Use `ctest --test-dir build --output-on-failure` for model checks. Use the documented smoke commands for changes to QML wiring, rendering or outputs. See [testing](../docs/testing.md) for commands and profile isolation.

Do not point fixtures at normal application data. Use QTemporaryDir for model data and the verification profile for GUI scenarios. Keep asynchronous captures alive explicitly; do not capture a helper's local variables by reference after it returns. A pass message must also be followed by a successful process exit.
