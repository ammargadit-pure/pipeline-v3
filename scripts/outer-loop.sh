#!/usr/bin/env bash
set -euo pipefail

MAX_ITERATIONS="${1:-30}"
MAX_PARALLEL="${2:-3}"
MAX_TASKS="${3:-0}"  # 0 = unlimited, N = stop after N tasks complete
PLAN_FILE="phase-plan.json"

# Track how many tasks we've completed this run
TASKS_COMPLETED_THIS_RUN=0
INITIAL_DONE=$(jq '[.phases[].tasks[] | select(.status == "complete")] | length' "$PLAN_FILE")

echo "╔══════════════════════════════════════════════════════════════╗"
echo "║  Pipeline v3 — Build→Test→Judge with Worktree Isolation    ║"
echo "║  Max iterations: ${MAX_ITERATIONS}  |  Max parallel: ${MAX_PARALLEL}  |  Max tasks: ${MAX_TASKS:-unlimited}  ║"
echo "╚══════════════════════════════════════════════════════════════╝"

# Ensure we're on main
git checkout main 2>/dev/null || true

find_eligible_tasks() {
  # Find pending tasks whose blockedBy are all complete
  jq -r '
    [.phases[].tasks[] | {id, status}] as $statuses |
    .phases[].tasks[] |
    select(.status == "pending") |
    . as $task |
    select(
      ($task.blockedBy | length) == 0 or
      ([$task.blockedBy[] | . as $dep | $statuses[] | select(.id == $dep and .status == "complete")] | length) == ($task.blockedBy | length)
    ) |
    "\(.id)|\(.skill)"
  ' "$PLAN_FILE"
}

find_dev_complete_tasks() {
  jq -r '.phases[].tasks[] | select(.status == "dev_complete") | .id' "$PLAN_FILE"
}

for iteration in $(seq 1 "$MAX_ITERATIONS"); do
  TOTAL=$(jq '[.phases[].tasks[]] | length' "$PLAN_FILE")
  DONE=$(jq '[.phases[].tasks[] | select(.status == "complete")] | length' "$PLAN_FILE")
  BLOCKED=$(jq '[.phases[].tasks[] | select(.status == "blocked")] | length' "$PLAN_FILE")
  
  echo ""
  echo "═══ Iteration ${iteration}/${MAX_ITERATIONS} [${DONE}/${TOTAL} complete, ${BLOCKED} blocked] ═══"

  # Step 1: Merge + test any dev_complete tasks
  DEV_COMPLETE=$(find_dev_complete_tasks)
  if [ -n "$DEV_COMPLETE" ]; then
    while IFS= read -r task_id; do
      echo "→ Merging + testing: ${task_id}"
      bash scripts/merge-and-test.sh "$task_id" || true
    done <<< "$DEV_COMPLETE"
  fi

  # Check if we've hit the MAX_TASKS limit
  if [ "$MAX_TASKS" -gt 0 ]; then
    CURRENT_DONE=$(jq '[.phases[].tasks[] | select(.status == "complete")] | length' "$PLAN_FILE")
    TASKS_COMPLETED_THIS_RUN=$((CURRENT_DONE - INITIAL_DONE))
    if [ "$TASKS_COMPLETED_THIS_RUN" -ge "$MAX_TASKS" ]; then
      echo ""
      echo "✅ MAX_TASKS limit reached: completed ${TASKS_COMPLETED_THIS_RUN} task(s) this run"
      echo "   Run again to continue with more tasks."
      break
    fi
  fi

  # CIRCUIT BREAKER: Stop new development if too many failures
  FAILED_COUNT=$(jq '[.phases[].tasks[] | select(.status == "failed")] | length' "$PLAN_FILE")
  
  if [ "$FAILED_COUNT" -gt 0 ]; then
    FAIL_RATIO=$((FAILED_COUNT * 100 / TOTAL))
    
    # Check consecutive failures using resolved_at timestamp (set by merge-and-test.sh)
    # Falls back to JSON position if timestamps not present
    CONSECUTIVE_FAILS=$(jq -r '
      [.phases[].tasks[] | select(.status == "failed" or .status == "complete") | select(.resolved_at != null)]
      | sort_by(.resolved_at)
      | .[-3:]
      | [.[] | select(.status == "failed")]
      | length
    ' "$PLAN_FILE" 2>/dev/null || echo "0")
    
    # Fallback: if no resolved_at timestamps yet, use simple count
    if [ "$CONSECUTIVE_FAILS" = "0" ] && [ "$FAILED_COUNT" -ge 3 ]; then
      CONSECUTIVE_FAILS="$FAILED_COUNT"
    fi
    
    # Big failure = 30%+ tasks failed OR 3 consecutive failures
    if [ "$FAIL_RATIO" -ge 30 ] || [ "$CONSECUTIVE_FAILS" -ge 3 ]; then
      echo ""
      echo "🛑 CIRCUIT BREAKER TRIPPED"
      echo "   Failed: ${FAILED_COUNT} tasks (${FAIL_RATIO}% of ${TOTAL})"
      echo "   Consecutive failures: ${CONSECUTIVE_FAILS}"
      echo ""
      echo "   Failed tasks:"
      jq -r '.phases[].tasks[] | select(.status == "failed") | "     ❌ \(.id): \(.title) (attempts: \(.attempts))"' "$PLAN_FILE"
      echo ""
      echo "   STOPPING new development to prevent cascading failures."
      echo "   Fix the failures before continuing."
      echo ""
      echo "   To investigate:"
      echo "     cat docs/test-reports/{task-id}.md"
      echo "     ls runs/                               # see run snapshots"
      echo "     cat runs/{run}/output.log              # see agent output"
      echo "     cat runs/{run}/diff.patch              # see what changed"
      echo ""
      echo "   After fixing, re-run: ./scripts/outer-loop.sh"
      break
    fi
  fi

  # Step 2: Launch new worktrees for eligible tasks (up to MAX_PARALLEL)
  ELIGIBLE=$(find_eligible_tasks)
  if [ -z "$ELIGIBLE" ] && [ -z "$DEV_COMPLETE" ]; then
    # Check if all done
    REMAINING=$(jq '[.phases[].tasks[] | select(.status != "complete" and .status != "blocked")] | length' "$PLAN_FILE")
    if [ "$REMAINING" -eq 0 ]; then
      echo "✅ ALL TASKS COMPLETE (or blocked)"
      break
    fi
    echo "No eligible tasks. Waiting..."
    sleep 5
    continue
  fi

  LAUNCHED=0
  while IFS='|' read -r task_id skill; do
    [ -z "$task_id" ] && continue
    [ "$LAUNCHED" -ge "$MAX_PARALLEL" ] && break
    
    echo "→ Launching worktree: ${task_id} (skill: ${skill})"
    
    # Launch in background for parallel execution
    bash scripts/launch-worktree.sh "$task_id" "$skill" 10 &
    
    LAUNCHED=$((LAUNCHED + 1))
  done <<< "$ELIGIBLE"

  # Wait for all background worktree jobs to finish
  wait

  # Sync dev_complete status from worktrees to main
  for wt in ../wt-*; do
    [ -d "$wt" ] || continue
    wt_task=$(basename "$wt" | sed 's/^wt-//')
    wt_status=$(jq -r --arg id "$wt_task" '.phases[].tasks[]? | select(.id == $id) | .status' "$wt/phase-plan.json" 2>/dev/null || echo "")
    if [ "$wt_status" = "dev_complete" ]; then
      main_status=$(jq -r --arg id "$wt_task" '.phases[].tasks[]? | select(.id == $id) | .status' "$PLAN_FILE")
      if [ "$main_status" != "dev_complete" ]; then
        echo "→ Syncing status: ${wt_task} = dev_complete"
        (
          flock -w 10 200
          jq --arg id "$wt_task" '(.phases[].tasks[]? | select(.id == $id)) |= (.status = "dev_complete")' "$PLAN_FILE" > tmp-sync.json && mv tmp-sync.json "$PLAN_FILE"
        ) 200>/tmp/pipeline-plan.lock
      fi
    fi
  done

  # Commit any status changes to avoid merge conflicts
  if ! git diff --quiet "$PLAN_FILE" 2>/dev/null; then
    git add "$PLAN_FILE" && git commit -m "chore: sync task status from worktrees" 2>/dev/null || true
  fi

  sleep 2
done

echo ""
echo "═══ FINAL STATUS ═══"
jq -r '.phases[].tasks[] | "\(.status | ascii_upcase)\t\(.id)\t\(.title)"' "$PLAN_FILE"
echo ""
echo "Git history:"
git log --oneline -20
echo ""
echo "Test results:"
CMD_TEST="${CMD_TEST:-npm run test}"
eval "$CMD_TEST" 2>&1 | tail -15 || true
