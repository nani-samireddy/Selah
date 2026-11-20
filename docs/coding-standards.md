# Coding standards

Use C++20, Qt Quick/QML and the established native UI controls. Keep behavior changes separate from broad formatting or organization changes where practical.

## Formatting

- Four spaces, UTF-8, LF line endings and a final newline. `.editorconfig` is authoritative for editors.
- C++ uses `.clang-format`: WebKit-derived layout, attached braces, braces for control flow, spaces around operators, separate function definitions and a 100-column target.
- QML uses `.qmlformat.ini`: four spaces, grouped declarations, blank lines between objects/functions and a 100-column target. Import order is preserved because changing it can change type resolution.
- Run `python3 scripts/format.py` to format, or `python3 scripts/format.py --check` to verify. The current baseline uses Apple clang-format 21 and qmlformat from Qt 6.12. Formatter upgrades may produce a separate formatting diff.

Install/use `clang-format` from your compiler toolchain and `qmlformat` from Qt. The helper also finds Apple's Command Line Tools formatter when it is not on PATH. Python 3 is required only for development helpers/data preparation.

## C++

Use PascalCase types, lowerCamelCase methods/local variables and `m_` members. Choose names that describe the domain; avoid one-letter variables in new production code except short loop coordinates. Use `const` when values do not change. Put the matching header first; let the formatter sort include groups.

Models own invariants and persistence; views call their APIs. Validate external data into a candidate before replacing active state. Keep work on the object's owning thread, and never pass a live QSqlDatabase connection to another thread. Use unique connection names and release queries/database handles before removing a connection.

Use QObject ownership or RAII deliberately. C++ stack controllers exposed to QML must retain C++ ownership. Prefer `QPointer` for QObject references that may disappear, and `std::unique_ptr` for exclusively owned non-QObject resources. Document exceptions such as asynchronous GPU readback lifetimes.

Do not change persisted keys, resource URLs or file formats as incidental cleanup. A schema change needs a version/migration strategy and round-trip/data-loss checks. Keep asynchronous callbacks' captured values alive; a helper returning before its timer fires must not leave references to local indexes or temporary strings.

## QML

One reusable component per file; organize by feature. Use required root properties and explicit signals for dependencies. Keep persisted state in the C++ model. UI-only state such as current preview index, focus, draft input and drag geometry belongs in QML.

Use `Layout.*` or implicit sizes for layout-managed items. Use anchors for free-positioned siblings; avoid assigning a competing width/height. Give delegates explicit IDs and required model properties rather than relying on dynamic parent chains. Reuse `StudioButton`, `StudioIcon`, library inputs and shared rendering components.

Do not copy the slide rendering pipeline into a new view. Preserve stable delegate identities while editing and disable video decoding in thumbnails. Keep keyboard shortcuts scoped so typing in an input or operating a modal picker cannot advance the live service.

## Comments and documentation

Comment ownership, threading, invariants, non-obvious math and compatibility decisions. Do not narrate obvious assignments or add a comment to every line. Explain why a workaround is needed and its limit. Update the relevant module README and feature guide when responsibilities, behavior or formats change.

Split a file when it mixes independent responsibilities; do not split solely to meet an arbitrary line count. Keep related code together and use named helpers to make complex conditions and transformations readable.

## Validation before handoff

Run formatting, QML lint and the model suite. For changes to rendering, component boundaries or model/view wiring, run the relevant visual smoke check and inspect its artifacts. Confirm the process exits successfully; a “passed” log line is insufficient if shutdown crashes. Use test profiles and temporary directories, never the normal user's recovery file for fixtures. See [testing](testing.md).
