#!/usr/bin/env bash
# NOTE: No set -e here — we handle every error explicitly so custom messages print
set -uo pipefail

# ─── CUSTOMIZE THESE FOR YOUR STACK ───
CMD_TYPECHECK="${CMD_TYPECHECK:-npm run typecheck}"    # or: mypy src/ | go vet ./... | cargo check
CMD_TEST="${CMD_TEST:-npm run test}"                    # or: pytest | go test ./... | cargo test
TEST_FILE_PATTERN="${TEST_FILE_PATTERN:-*.test.ts}"     # or: test_*.py | *_test.go | *_test.rs
TEST_DIR="${TEST_DIR:-tests}"                           # or: test/ | src/ (for Go)
UNIT_TEST_DIR="${UNIT_TEST_DIR:-tests/unit}"            # where mocks are allowed
MOCK_PATTERNS="${MOCK_PATTERNS:-vi\.mock\|vi\.fn()\|jest\.mock\|jest\.fn()\|sinon\.\|\.stub(\|vi\.spyOn\|jest\.spyOn}"
# For Python: MOCK_PATTERNS="@patch\|MagicMock\|mock_\|@mock\|unittest\.mock"
# For Go: MOCK_PATTERNS="gomock\|mockgen\|MockController"
SKIP_PATTERNS="${SKIP_PATTERNS:-test\.skip\|it\.skip\|describe\.skip\|xit(\|xdescribe(\|xtest(\|\.todo(\|\.skipIf(\|\.runIf(}"
# For Python: SKIP_PATTERNS="@pytest\.mark\.skip\|pytest\.skip\|unittest\.skip\|@pytest\.mark\.skipIf"
# For Go: SKIP_PATTERNS="t\.Skip("
ASSERT_PATTERN="${ASSERT_PATTERN:-expect(}"
# For Python: ASSERT_PATTERN="assert "
# For Go: ASSERT_PATTERN="assert\.\|require\."
TEST_BLOCK_PATTERN="${TEST_BLOCK_PATTERN:-it(}"
# For Python: TEST_BLOCK_PATTERN="def test_"
# For Go: TEST_BLOCK_PATTERN="func Test"
# ─── END CUSTOMIZATION ───

FAILED=0

# Check 1: typecheck / lint
echo "=== VALIDATE: typecheck ==="
if ! eval "$CMD_TYPECHECK" 2>&1; then
  echo "FAIL: typecheck / lint failed"
  FAILED=1
fi

# Check 2: tests pass
echo "=== VALIDATE: test ==="
if ! eval "$CMD_TEST" 2>&1; then
  echo "FAIL: tests failed"
  FAILED=1
fi

[ "$FAILED" -ne 0 ] && exit 1

# Check 3: no skipped tests
echo "=== VALIDATE: no skipped tests ==="
SKIPPED=$(grep -rn "$SKIP_PATTERNS" "$TEST_DIR" 2>/dev/null | wc -l || echo "0")
if [ "$SKIPPED" -gt 0 ]; then
  echo "FAIL: found $SKIPPED skipped tests. Skipped tests are not allowed."
  grep -rn "$SKIP_PATTERNS" "$TEST_DIR" 2>/dev/null
  exit 1
fi

# Check 4: no mocks outside unit tests
echo "=== VALIDATE: no mocks outside unit tests ==="
NON_UNIT_DIRS=$(find "$TEST_DIR" -type d -mindepth 1 ! -path "$UNIT_TEST_DIR" ! -path "$UNIT_TEST_DIR/*" 2>/dev/null | tr '\n' ' ')
if [ -n "$NON_UNIT_DIRS" ]; then
  MOCK_VIOLATIONS=$(grep -rn "$MOCK_PATTERNS" $NON_UNIT_DIRS 2>/dev/null | wc -l || echo "0")
  if [ "$MOCK_VIOLATIONS" -gt 0 ]; then
    echo "FAIL: found $MOCK_VIOLATIONS mock violations outside $UNIT_TEST_DIR"
    grep -rn "$MOCK_PATTERNS" $NON_UNIT_DIRS 2>/dev/null
    exit 1
  fi
fi

# Check 5: assertion density — HARD FAIL, not just a warning
echo "=== VALIDATE: assertion density ==="
DENSITY_FAIL=0
for f in $(find "$TEST_DIR" -name "$TEST_FILE_PATTERN" 2>/dev/null); do
  TESTS=$(grep -c "$TEST_BLOCK_PATTERN" "$f" 2>/dev/null || echo 0)
  EXPECTS=$(grep -c "$ASSERT_PATTERN" "$f" 2>/dev/null || echo 0)
  if [ "$TESTS" -gt 0 ] && [ "$EXPECTS" -lt "$((TESTS * 2))" ]; then
    echo "FAIL: $f has $TESTS tests but only $EXPECTS assertions (min: $((TESTS * 2)))"
    DENSITY_FAIL=1
  fi
done
if [ "$DENSITY_FAIL" -ne 0 ]; then
  echo "FAIL: assertion density below minimum (2 per test block)"
  exit 1
fi

# Check 6: shared-contracts.md was NOT modified by developer agents
echo "=== VALIDATE: contract integrity ==="
if [ -f ".contract-hash" ]; then
  CURRENT_HASH=$(sha256sum shared-contracts.md 2>/dev/null | cut -d' ' -f1)
  EXPECTED_HASH=$(cat .contract-hash 2>/dev/null)
  if [ "$CURRENT_HASH" != "$EXPECTED_HASH" ]; then
    echo "FAIL: shared-contracts.md was modified during development!"
    echo "       Only the architect agent may change contracts."
    echo "       Restore with: git checkout main -- shared-contracts.md"
    exit 1
  fi
fi

# Check 7: quality gate thresholds in phase-plan.json not tampered
echo "=== VALIDATE: quality gate integrity ==="
if [ -f ".gates-hash" ]; then
  CURRENT_GATES=$(jq -c '.quality_gates' phase-plan.json 2>/dev/null | sha256sum | cut -d' ' -f1)
  EXPECTED_GATES=$(cat .gates-hash 2>/dev/null)
  if [ "$CURRENT_GATES" != "$EXPECTED_GATES" ]; then
    echo "FAIL: quality_gates in phase-plan.json were modified!"
    echo "       Agents must not change quality thresholds."
    exit 1
  fi
fi

echo "=== VALIDATE: PASS ==="
exit 0
