#!/usr/bin/env bash
set -euo pipefail

TASK_ID="${1:?Usage: loop.sh <task-id> <skill>}"
SKILL="${2:?Usage: loop.sh <task-id> <skill>}"
MAX_ITERATIONS="${3:-10}"
RUN_NUM="${4:-1}"
ITERATION=0

PROJECT_ROOT="$(pwd)"
LOG_FILE="scripts/logs/${TASK_ID}.log"
LOCK_FILE="/tmp/pipeline-plan.lock"
mkdir -p scripts/logs
mkdir -p runs

START_TIME=$(date +%s)

# ─── CUSTOMIZE FOR YOUR STACK ───
CMD_TYPECHECK="${CMD_TYPECHECK:-npm run typecheck}"
CMD_INSTALL="${CMD_INSTALL:-npm install}"
DEPS_DIR="${DEPS_DIR:-node_modules}"   # or: .venv | vendor | target
# ─── END CUSTOMIZATION ───

# PRE-FLIGHT: Does the baseline build?
echo "Pre-flight check: $CMD_TYPECHECK..."
if ! eval "$CMD_TYPECHECK" 2>&1; then
  echo "ABORT: Baseline does not pass type/lint check. Fix before running loop."
  exit 1
fi

# Install deps if needed
if [ -n "$DEPS_DIR" ] && [ ! -d "$DEPS_DIR" ]; then
  echo "Installing dependencies..."
  eval "$CMD_INSTALL"
fi

# INTEGRITY: Lock down shared-contracts.md and quality gates before agent starts
# Agents can read these but validate.sh will catch any modifications
sha256sum shared-contracts.md 2>/dev/null | cut -d' ' -f1 > .contract-hash 2>/dev/null || true
jq -c '.quality_gates' phase-plan.json 2>/dev/null | sha256sum | cut -d' ' -f1 > .gates-hash 2>/dev/null || true

PROMPT="You are a fresh autonomous agent with NO memory of previous work.

YOUR TASK: ${TASK_ID}

CRITICAL RULES:
1. Read CLAUDE.md, shared-contracts.md, working-memory.md FIRST.
2. Read phase-plan.json — find your task '${TASK_ID}'.
3. Run: git log --oneline -10 to see what exists.
4. Follow .claude/skills/${SKILL}/SKILL.md step by step.
5. Run validation: $CMD_TYPECHECK && $CMD_TEST (see CLAUDE.md for your stack's commands)
6. Do NOT consider yourself done unless BOTH commands exit with code 0.
7. Do NOT use test.skip(). Do NOT use mocks outside tests/unit/.
8. Follow shared-contracts.md EXACTLY. Do NOT invent new names.
9. When done, update phase-plan.json: set '${TASK_ID}' status to 'dev_complete'.
10. Append learnings to working-memory.md.
11. Also write learnings to runs/ folder if it exists: runs/*-${TASK_ID}/learnings.md"

for ITERATION in $(seq 1 "$MAX_ITERATIONS"); do
  echo ""
  echo "═══ RALPH LOOP — Task: ${TASK_ID} — Iteration ${ITERATION}/${MAX_ITERATIONS} ═══"
  
  set +e
  # Wall-clock timeout: kill agent if single iteration exceeds 10 minutes
  ITER_TIMEOUT="${ITER_TIMEOUT:-600}"
  timeout "$ITER_TIMEOUT" bash -c "echo '$PROMPT' | claude --print --dangerously-skip-permissions --output-format json --max-turns 20 2>&1" | tee -a "$LOG_FILE"
  AGENT_EXIT=$?
  set -e

  if [ "$AGENT_EXIT" -eq 124 ]; then
    echo "⚠ TIMEOUT: Agent exceeded ${ITER_TIMEOUT}s wall-clock limit for iteration ${ITERATION}"
  fi

  # BASH SAFETY NET: auto-commit (agents are unreliable at committing)
  git add -A 2>/dev/null || true
  if ! git diff --cached --quiet 2>/dev/null; then
    git commit -m "ralph(${TASK_ID}): auto-commit from iteration ${ITERATION}" 2>/dev/null || true
  fi

  # Check if task is marked dev_complete (locked read to prevent race)
  TASK_STATUS=$(
    flock -w 10 "$LOCK_FILE" jq -r --arg id "$TASK_ID" '.phases[].stories[]? | select(.id == $id) | .status' phase-plan.json 2>/dev/null || echo "unknown"
  )
  
  if [ "$TASK_STATUS" = "dev_complete" ]; then
    # VERIFY: don't trust the agent, verify ourselves
    echo "Agent says done. Verifying..."
    if bash scripts/validate.sh 2>&1; then
      echo "✅ VALIDATED: ${TASK_ID} — iteration ${ITERATION}"
      # Snapshot this run
      bash scripts/snapshot-run.sh "$RUN_NUM" "$TASK_ID" "$PROMPT" "$LOG_FILE" "0" "$START_TIME" "$ITERATION" 2>/dev/null || true
      exit 0
    else
      echo "❌ Validation failed. Agent marked done but build/tests broken."
      # Reset status back to in_progress (locked write)
      (
        flock -w 10 200
        jq --arg id "$TASK_ID" '(.phases[].stories[]? | select(.id == $id)) |= (.status = "in_progress")' phase-plan.json > tmp-${TASK_ID}.json && mv tmp-${TASK_ID}.json phase-plan.json
      ) 200>"$LOCK_FILE"
      # Continue loop
    fi
  fi

  # Check for max iterations
  if [ "$ITERATION" -ge "$MAX_ITERATIONS" ]; then
    echo "⚠ Max iterations reached for ${TASK_ID}"
    (
      flock -w 10 200
      jq --arg id "$TASK_ID" '(.phases[].stories[]? | select(.id == $id)) |= (.status = "blocked" | .attempts += 1)' phase-plan.json > tmp-${TASK_ID}.json && mv tmp-${TASK_ID}.json phase-plan.json
    ) 200>"$LOCK_FILE"
    # Snapshot the failed run
    bash scripts/snapshot-run.sh "$RUN_NUM" "$TASK_ID" "$PROMPT" "$LOG_FILE" "1" "$START_TIME" "$ITERATION" 2>/dev/null || true
    exit 1
  fi

  sleep 2
done
