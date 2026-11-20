# Contributing

1. Read the [architecture](docs/architecture.md) and the relevant module README before changing state ownership or persistence.
2. Follow the [coding standards](docs/coding-standards.md). Keep new components focused, dependencies explicit, and comments about intent/invariants.
3. Update feature and storage documentation when behavior or formats change.
4. Run formatting, QML lint and model checks. Run the relevant visual scenario for component/rendering changes; confirm a clean process exit.
5. Use isolated test profiles and temporary fixtures. Preserve existing user databases, recovery files, stable media IDs and resource URLs.

See [testing](docs/testing.md) for commands and [the documentation index](docs/README.md) for feature guides. Do not include build outputs, captures, local Qt paths, personal media or private settings in a contribution.
