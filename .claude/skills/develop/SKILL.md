---
name: develop
description: "Build ONE feature and its developer tests. Ralph loop until green."
---

# Developer Agent

CRITICAL RULES:
1. Read CLAUDE.md, shared-contracts.md, working-memory.md FIRST.
2. Read phase-plan.json — find YOUR task. Implement ONLY that task.
3. Follow shared-contracts.md EXACTLY. Do NOT invent new names, types, or response shapes.
4. Write the feature code: route + service + types.
5. Write unit tests (mocks OK) in tests/unit/.
6. Write integration tests (NO mocks, real DB) in tests/integration/.
7. Run validation: $CMD_TYPECHECK && $CMD_TEST (see CLAUDE.md for commands)
8. Do NOT mark task as 'dev_complete' unless BOTH commands exit with code 0.
9. Do NOT use test.skip() / @pytest.mark.skip / t.Skip() anywhere. Skipped tests = failure.
10. Commit with: feat({module}): {description}

## Step-by-step:

### 1. Understand your task
````bash
cat phase-plan.json | jq '.phases[].stories[] | select(.id == "YOUR_TASK_ID")'
````

### 2. Read existing code
````bash
git log --oneline -10
ls src/ tests/ 2>/dev/null    # or: ls app/ lib/ test/ cmd/ internal/
cat shared-contracts.md
````

### 3. Build the feature
Create files according to tech-spec.md. Common patterns:
- JS/TS: src/routes/{module}.ts + src/services/{module}.service.ts
- Python: app/routers/{module}.py + app/services/{module}.py
- Go: internal/{module}/handler.go + internal/{module}/service.go
- Use EXACT types from shared-contracts.md
- Wire into the main entry point (index.ts / main.py / main.go / etc.)

### 4. Write unit tests
- Mocks ARE allowed here (ONLY here)
- Test business logic in isolation
- Minimum 5 tests per service function
- Minimum 2 assertions per test

### 5. Write integration tests
- NO MOCKS. ZERO TOLERANCE.
- Real database (in-memory or test instance)
- Real HTTP requests (test client or localhost)
- Test: create, read, update, delete
- Test: validation errors
- Test: not found
- Test: unauthorized
- Verify DB state after operations

### 6. Validate
````bash
$CMD_TYPECHECK    # Must pass (exit code 0)
$CMD_TEST         # Must pass (exit code 0)
````
BOTH must exit with code 0.

### 7. Update phase-plan.json
Set your task status to "dev_complete".

### 8. Update working-memory.md
Append what you learned:
- Patterns that worked
- Gotchas discovered
- Helpful for other agents
