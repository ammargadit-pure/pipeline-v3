# Verification Plan

## Global Checks (run for EVERY task)
# ─── CUSTOMIZE THESE FOR YOUR STACK ───
````bash
$CMD_TEST           # Run all tests (npm run test / pytest / go test ./... / cargo test)
$CMD_TYPECHECK      # Type/lint check (npm run typecheck / mypy src/ / go vet ./... / cargo check)

# Mock contamination check — patterns depend on your language:
# JS/TS:   grep -rn "vi\.mock\|vi\.fn\|jest\.mock" tests/integration/ tests/e2e/
# Python:  grep -rn "@patch\|MagicMock" tests/integration/ tests/e2e/
# Go:      grep -rn "gomock\|MockController" *_test.go (outside unit dir)
grep -rn "$MOCK_PATTERNS" tests/integration/ tests/e2e/ tests/contract/ tests/property/ && echo "FAIL: mocks outside unit tests" && exit 1

# Skip test check:
# JS/TS:   grep -rn "test\.skip\|it\.skip\|describe\.skip" tests/
# Python:  grep -rn "@pytest.mark.skip\|pytest.skip" tests/
# Go:      grep -rn "t\.Skip(" *_test.go
grep -rn "$SKIP_PATTERNS" tests/ && echo "FAIL: skipped tests found" && exit 1
````

## Per-Task Verification
[Human fills this. Examples for different stacks:]

### Example: API endpoint (any language)
````bash
# Start server, test health endpoint
$CMD_DEV &
sleep 2
curl -s -o /dev/null -w "%{http_code}" http://localhost:3000/api/health | grep -q 200
kill %1
````

### Example: Auth flow (any language)
````bash
bash scripts/verify/check-auth-flow.sh
````

### Example: Python-specific
````bash
pytest tests/integration/test_users.py -v
````

### Example: Go-specific
````bash
go test ./internal/users/... -v -run TestCreateUser
````

## Custom Verification Methods
[Human can add any of these:]
- Bash scripts
- Docker compose up → run tests → docker compose down
- SSH to staging → deploy → smoke test
- Playwright MCP → browser tests
- curl commands against live API
- GitLab CI trigger → wait → check result
- Any MCP server you have configured

## Human Checkpoints
- After Phase 1 (foundation): STOP, ask human to review schema
- After all builds: STOP, ask human to verify
- After judge: STOP, show quality report
