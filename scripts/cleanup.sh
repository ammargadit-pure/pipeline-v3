#!/usr/bin/env bash
# Remove all worktrees
for wt in ../wt-*; do
  [ -d "$wt" ] && git worktree remove "$wt" --force 2>/dev/null
done
git worktree prune
echo "Cleaned up all worktrees"
