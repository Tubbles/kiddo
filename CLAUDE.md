# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Kiddo is a minimal 2D shapes game for toddlers (ages 2-4), focused on hand-eye coordination and learning to use a gamepad. See [ARCHITECTURE.md](ARCHITECTURE.md) for the full system design.

## Building

```bash
# Install dependencies and build
conan install . --output-folder=build --build=missing
conan build .

# Run
./build/Release/kiddo

# Run tests
./build/Release/test/kiddo_tests
```

## Dependencies

- C11 compiler (gcc or clang)
- Conan 2 (package manager — drives CMake)
- CMake (generated/invoked by Conan)
- raylib (graphics, input, audio)
- Unity (ThrowTheSwitch — unit test framework)
- fff.h (Fake Function Framework — mocking)

## Coding Style

- **No OOP patterns.** This is C — think plain structs and functions.
- **Small, focused functions.** Aim for 5-10 lines per function. Extract logic into named helpers rather than writing long functions.
- **Pure functions where possible.** Functions should take inputs, return outputs, and avoid side effects. Side effects (I/O, rendering, audio) should be pushed to the edges — thin wrapper functions that call pure logic.
- **Data-oriented design.** Game state is plain structs. Logic operates on that data. Data flow is one-directional: input -> state -> render.
- **Test everything with Unity + fff.h.** Every non-trivial pure function should have corresponding tests in `test/`. If a function is hard to test, it probably does too much.

## Git Workflow

- **Always commit and push when you're done with a task.** Do not wait to be asked — committing and pushing is part of completing the work.
- Create small, focused commits as you go so changes are easy to review and revert.
- Each commit should address a single concern (one bug fix, one feature, one refactor).
- Use a succinct imperative commit title (e.g. "Add retry logic for API calls").
- Include gotchas, caveats, or non-obvious side effects in the commit message body.
- Never add "Co-Authored-By" lines or email addresses to commit messages.
- Push freely without asking, but never use `git push --force` or any force-push variant.
- **Keep all documentation up to date.** When changing behavior, update CLAUDE.md and code comments in the same commit. Stale docs are worse than no docs.
