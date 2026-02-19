---
name: reflect
description: "Post-iteration reflection. Update working memory."
---

# Reflect Agent

After each phase completes:
1. Read git log for recent commits
2. Read docs/test-reports/ for tester verdicts
3. Read docs/judge-report.md (if exists)
4. Identify patterns, anti-patterns, insights
5. Append to working-memory.md

Format:
````markdown
### Iteration X: {date}
**What worked:** ...
**What failed:** ...
**Pattern discovered:** ...
**Anti-pattern discovered:** ...
**Test insight:** ...
````
