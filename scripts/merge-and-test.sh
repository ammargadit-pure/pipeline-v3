#!/usr/bin/env bash
set -euo pipefail

TASK_ID="${1:?Usage: merge-and-test.sh <task-id>}"
WORKTREE_DIR="../wt-${TASK_ID}"

echo "=== Merging ${TASK_ID} to main ==="

# Go to main repo
cd "$(git worktree list | head -1 | awk '{print $1}')"
git checkout main 2>/dev/null || true

# Merge the branch
git merge "$TASK_ID" --no-edit 2>&1
MERGE_EXIT=$?

if [ $MERGE_EXIT -ne 0 ]; then
  echo "FAIL: Merge conflict for ${TASK_ID}"
  git merge --abort 2>/dev/null || true
  (
    flock -w 10 200
    jq --arg id "$TASK_ID" '(.phases[].stories[]? | select(.id == $id)) |= (.status = "failed")' phase-plan.json > tmp-${TASK_ID}.json && mv tmp-${TASK_ID}.json phase-plan.json
  ) 200>/tmp/pipeline-plan.lock
  exit 1
fi

# Run validation on merged code
echo "=== Running validation on merged code ==="
CMD_INSTALL="${CMD_INSTALL:-npm install}"
eval "$CMD_INSTALL" --silent 2>/dev/null || eval "$CMD_INSTALL" 2>/dev/null || true
if bash scripts/validate.sh 2>&1; then
  echo "=== Running tester verification ==="
  
  # Run per-task verification from verification-plan.md (if defined)
  TASK_VERIFY_SCRIPT="scripts/verify/${TASK_ID}.sh"
  if [ -f "$TASK_VERIFY_SCRIPT" ]; then
    echo "→ Running per-task verification: $TASK_VERIFY_SCRIPT"
    if ! bash "$TASK_VERIFY_SCRIPT" 2>&1; then
      echo "FAIL: Per-task verification failed for ${TASK_ID}"
      git revert HEAD --no-edit 2>/dev/null || true
      (
        flock -w 10 200
        jq --arg id "$TASK_ID" '(.phases[].stories[]? | select(.id == $id)) |= (.status = "failed" | .attempts += 1)' phase-plan.json > tmp-${TASK_ID}.json && mv tmp-${TASK_ID}.json phase-plan.json
      ) 200>/tmp/pipeline-plan.lock
      exit 1
    fi
  else
    echo "→ No per-task verification script at $TASK_VERIFY_SCRIPT (skipping custom checks)"
    echo "  To add: create scripts/verify/${TASK_ID}.sh with curl/docker/custom commands"
  fi
  
  # Mark complete (locked) with resolved_at timestamp for circuit breaker ordering
  RESOLVED_AT=$(date -u +%Y-%m-%dT%H:%M:%SZ)
  (
    flock -w 10 200
    jq --arg id "$TASK_ID" --arg ts "$RESOLVED_AT" '(.phases[].stories[]? | select(.id == $id)) |= (.status = "complete" | .passes = true | .resolved_at = $ts)' phase-plan.json > tmp-${TASK_ID}.json && mv tmp-${TASK_ID}.json phase-plan.json
  ) 200>/tmp/pipeline-plan.lock
  
  # Write test report
  mkdir -p docs/test-reports
  echo "# Test Report: ${TASK_ID}" > "docs/test-reports/${TASK_ID}.md"
  echo "## Verdict: PASS" >> "docs/test-reports/${TASK_ID}.md"
  echo "## Date: $(date -u +%Y-%m-%dT%H:%M:%SZ)" >> "docs/test-reports/${TASK_ID}.md"
  CMD_COVERAGE="${CMD_COVERAGE:-npm run test:coverage}"
  eval "$CMD_COVERAGE" 2>&1 | tail -20 >> "docs/test-reports/${TASK_ID}.md" || true
  
  git add -A && git commit -m "test(${TASK_ID}): verified and merged" 2>/dev/null || true
  echo "✅ ${TASK_ID}: merged + verified"
else
  echo "FAIL: Validation failed after merge"
  git revert HEAD --no-edit 2>/dev/null || true
  (
    flock -w 10 200
    jq --arg id "$TASK_ID" '(.phases[].stories[]? | select(.id == $id)) |= (.status = "failed" | .attempts += 1)' phase-plan.json > tmp-${TASK_ID}.json && mv tmp-${TASK_ID}.json phase-plan.json
  ) 200>/tmp/pipeline-plan.lock
  exit 1
fi

# Cleanup worktree
git worktree remove "$WORKTREE_DIR" 2>/dev/null || true
git branch -d "$TASK_ID" 2>/dev/null || true
