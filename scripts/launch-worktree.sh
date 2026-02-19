#!/usr/bin/env bash
set -euo pipefail

TASK_ID="${1:?Usage: launch-worktree.sh <task-id> <skill>}"
SKILL="${2:?Usage: launch-worktree.sh <task-id> <skill>}"
MAX_ITER="${3:-10}"

WORKTREE_DIR="../wt-${TASK_ID}"
LOCK_FILE="/tmp/pipeline-plan.lock"

# Create worktree
echo "Creating worktree: ${WORKTREE_DIR} on branch ${TASK_ID}"
git worktree add "$WORKTREE_DIR" -b "$TASK_ID" 2>/dev/null || {
  # Branch might already exist
  git worktree add "$WORKTREE_DIR" "$TASK_ID" 2>/dev/null || {
    echo "Worktree already exists, using it"
  }
}

# Copy env files if they exist
[ -f .env ] && cp .env "$WORKTREE_DIR/.env" 2>/dev/null || true

# Install deps in worktree — FAIL LOUDLY if install fails
cd "$WORKTREE_DIR"
CMD_INSTALL="${CMD_INSTALL:-npm install}"
DEPS_DIR="${DEPS_DIR:-node_modules}"
if [ -n "$DEPS_DIR" ] && [ ! -d "$DEPS_DIR" ]; then
  echo "Installing dependencies in worktree..."
  if ! eval "$CMD_INSTALL" 2>&1; then
    echo "ABORT: Dependency install failed in worktree ${WORKTREE_DIR}"
    echo "       Fix dependencies before running pipeline."
    exit 1
  fi
fi

# Mark task as in_progress (FILE-LOCKED to prevent race with parallel worktrees)
(
  flock -w 10 200 || { echo "WARN: Could not acquire lock for phase-plan.json"; exit 1; }
  jq --arg id "$TASK_ID" '(.phases[].tasks[]? | select(.id == $id)) |= (.status = "in_progress")' phase-plan.json > tmp-${TASK_ID}.json && mv tmp-${TASK_ID}.json phase-plan.json
) 200>"$LOCK_FILE"

# Calculate run number (also locked to prevent parallel collision)
(
  flock -w 10 200 || true
  RUN_NUM=$(( $(ls -d runs/???-* 2>/dev/null | wc -l) + 1 ))
  echo "$RUN_NUM" > "/tmp/pipeline-run-${TASK_ID}.num"
) 200>"$LOCK_FILE"
RUN_NUM=$(cat "/tmp/pipeline-run-${TASK_ID}.num" 2>/dev/null || echo "1")

# Run the ralph loop inside the worktree
bash scripts/loop.sh "$TASK_ID" "$SKILL" "$MAX_ITER" "$RUN_NUM"
