# Project Context

## What This Is
[Planner will fill this after requirements discussion]

## Tech Stack
[Architect will fill this]

## How Agents Work

### Context Loading (EVERY agent does this FIRST)
1. Read this file (CLAUDE.md)
2. Read shared-contracts.md — naming, types, response shapes
3. Read test-strategy.md — testing approach, infrastructure, dependencies
4. Read working-memory.md — learnings from past iterations
5. Read phase-plan.json — find YOUR task
6. Run: git log --oneline -10
7. Run: ls src/ tests/

### CRITICAL RULES (apply to ALL agents)
1. Follow shared-contracts.md EXACTLY. Do NOT invent new names.
2. Do NOT mark a task complete unless validation passes with exit code 0.
3. Do NOT use test.skip(). Skipped tests are treated as failures.
4. Do NOT use mocks outside tests/unit/. Zero tolerance.
5. Every test must have at least 2 meaningful assertions.
6. Commit messages: feat(module): description OR test(module): description

## Commands
# ─── CUSTOMIZE THESE FOR YOUR STACK ───
# These are the commands every agent uses. Replace with your equivalents.
#
# Node.js/TypeScript example:
#   CMD_TEST="npm run test"
#   CMD_TYPECHECK="npm run typecheck"
#   CMD_COVERAGE="npm run test:coverage"
#   CMD_DEV="npm run dev"
#   CMD_INSTALL="npm install"
#
# Python example:
#   CMD_TEST="pytest"
#   CMD_TYPECHECK="mypy src/"
#   CMD_COVERAGE="pytest --cov=src"
#   CMD_DEV="uvicorn src.main:app --reload"
#   CMD_INSTALL="pip install -r requirements.txt"
#
# Go example:
#   CMD_TEST="go test ./..."
#   CMD_TYPECHECK="go vet ./..."
#   CMD_COVERAGE="go test -cover ./..."
#   CMD_DEV="go run ."
#   CMD_INSTALL="go mod download"

- $CMD_TEST            → Run all tests
- $CMD_TEST_UNIT       → Unit tests only (if separate)
- $CMD_TEST_INTEG      → Integration tests only (if separate)
- $CMD_COVERAGE        → With coverage report
- $CMD_TYPECHECK       → Static analysis / type checking
- $CMD_DEV             → Start dev server
