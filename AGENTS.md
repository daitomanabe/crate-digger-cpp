# AGENTS.md

## Repository Role

- Name: `crate-digger-cpp`
- Role: `repo`
- Public surface: website, webpage

## Working Rules

- This root is the canonical git root for this repository.
- Nested repositories must stay as submodules, never as raw nested `.git` directories.
- Do not merge sibling repositories back into this tree.
- Preserve generated bilingual documentation unless you are intentionally updating it.

## Important Paths

- `README.md` is the human-facing overview.
- `FILE-STRUCTURE.md` documents the normalized layout.
- Public materials live in `website/`, `webpage/`, `ui/`, or root `index.html`.

## Submodules

- None

## Safe Changes

- Update `.gitignore` by appending to the generated block when needed.
- Keep vendor dependencies isolated as submodules.
- Prefer small, repo-local commits.
