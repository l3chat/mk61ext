# Project Memory

This file is the tracked, low-noise project memory for durable decisions and conventions.

## Scope

- Use this file for long-lived, shared context that should survive machines, sessions, and contributors.
- Keep `.agents/` and `AGENTS.md` as local working memory (faster, higher-churn, not tracked).
- Keep `_archive/` as historical reference only.

## Update Rules

- Add entries only for meaningful milestones, design decisions, and durable workflow agreements.
- Prefer short bullets with concrete file or behavior references.
- Avoid per-session chatter and temporary debugging notes.

## Current Durable Decisions

- The project uses a dual-memory approach:
  - tracked shared memory in this file,
  - local high-churn memory in `.agents/`.
- Program-mode `DROP` is part of the active core opcode map as opcode `0x27`.
- Input handling prioritizes prefix keys (`k`, `p`) within the same keypad scan frame to stabilize quick shifted combos like `K*`.

## Recent Durable Changes

### 2026-04-22

- Program-mode pending input is now surfaced directly in both places:
  - listing cursor row shows recorder pending preview (`GSB __`, `GSB 1_`, etc.),
  - status line shows unfinished command text (`IN GSB ?`, `IN XM ?`) instead of generic `PND`.
- Program-mode status layout was simplified:
  - removed `Lxx` length from the status-left payload,
  - status-left now favors cursor/edit-mode plus unfinished/last-command context.
- Power visibility is now explicit in program mode status-right:
  - voltage remains visible,
  - when running on battery, an estimated battery percentage is also shown (`V + %`).

### 2026-04-21

- Added full program-mode `DROP` support end-to-end:
  - recorder maps `K*` to opcode `0x27`,
  - VM decodes/lists `0x27` as `DROP`,
  - runner executes `0x27` using calculator `DROP` semantics.
- Added host regression coverage for:
  - opcode decode of `0x27`,
  - recorder mapping for `K*`,
  - runner execution behavior of a stored `DROP` step.

## Entry Template

```markdown
### YYYY-MM-DD

- Decision:
  - What was decided and why it is durable.
- Implementation:
  - Files changed and behavior impact.
- Validation:
  - Tests/build/hardware checks that passed.
- Follow-up (optional):
  - Any deferred but likely next step.
```
