#!/usr/bin/env bash
set -euo pipefail

# Usage: snapshot-run.sh <run-number> <task-id> <prompt-text> <log-file> <exit-code> <start-time> <iterations>

RUN_NUM="${1:?Usage: snapshot-run.sh <run-num> <task-id> <prompt> <log-file> <exit-code> <start-time> <iterations>}"
TASK_ID="${2}"
PROMPT_TEXT="${3}"
LOG_FILE="${4}"
EXIT_CODE="${5}"
START_TIME="${6}"
ITERATIONS="${7}"

# Zero-pad run number
RUN_DIR="runs/$(printf '%03d' "$RUN_NUM")-${TASK_ID}"
mkdir -p "${RUN_DIR}/context"

# 1. Save the exact prompt
echo "$PROMPT_TEXT" > "${RUN_DIR}/prompt.md"

# 2. Snapshot the context documents the agent was given
cp shared-contracts.md "${RUN_DIR}/context/shared-contracts.md" 2>/dev/null || true
cp phase-plan.json "${RUN_DIR}/context/phase-plan.json" 2>/dev/null || true
cp working-memory.md "${RUN_DIR}/context/working-memory.md" 2>/dev/null || true
cp test-strategy.md "${RUN_DIR}/context/test-strategy.md" 2>/dev/null || true
cp verification-plan.md "${RUN_DIR}/context/verification-plan.md" 2>/dev/null || true

# 3. Copy the output log
cp "$LOG_FILE" "${RUN_DIR}/output.log" 2>/dev/null || echo "(no log)" > "${RUN_DIR}/output.log"

# 4. Capture git diff of ALL changes this task made (from branch point, not just last commit)
BRANCH_POINT=$(git merge-base main HEAD 2>/dev/null || echo "HEAD~1")
git diff "$BRANCH_POINT" HEAD > "${RUN_DIR}/diff.patch" 2>/dev/null || echo "(no diff)" > "${RUN_DIR}/diff.patch"

# 5. Write result metadata
END_TIME=$(date +%s)
DURATION=$((END_TIME - START_TIME))

cat > "${RUN_DIR}/result.json" << ENDJSON
{
  "task_id": "${TASK_ID}",
  "run_number": ${RUN_NUM},
  "status": "$([ "$EXIT_CODE" -eq 0 ] && echo 'success' || echo 'failed')",
  "exit_code": ${EXIT_CODE},
  "iterations": ${ITERATIONS},
  "duration_seconds": ${DURATION},
  "started_at": "$(date -u -d @${START_TIME} +%Y-%m-%dT%H:%M:%SZ 2>/dev/null || date -u +%Y-%m-%dT%H:%M:%SZ)",
  "finished_at": "$(date -u +%Y-%m-%dT%H:%M:%SZ)"
}
ENDJSON

# 6. Placeholder for learnings (agent fills this, or stays empty)
echo "# Learnings: ${TASK_ID} (run ${RUN_NUM})" > "${RUN_DIR}/learnings.md"
echo "" >> "${RUN_DIR}/learnings.md"
echo "_Auto-generated. Agent appends learnings here during execution._" >> "${RUN_DIR}/learnings.md"

echo "📁 Run snapshot saved: ${RUN_DIR}"
