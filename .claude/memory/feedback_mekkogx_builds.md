---
name: MekkoGX build system is hands-off
description: Never modify mekkogx/ files; always build from project root Makefile. Don't investigate mekkogx internals when builds fail.
type: feedback
---

When a project uses MekkoGX, always build from the project root using `make <platform>`. Never modify files in the mekkogx/ folder. When builds fail, look at the project's own source and Makefile — not the mekkogx build system internals.

**Why:** MekkoGX is a shared/external build system. The user already knows how it works and doesn't want time wasted reading its internals.

**How to apply:** On build failures, check the project's src/ files, the root Makefile, and the actual compiler errors — not the mekkogx/*.mk plumbing.
