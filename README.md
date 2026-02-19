# Pipeline v3 — Multi-Agent Development System

## Quick Start
1. Clone this repo
2. Copy `.pipeline.env.example` to `.pipeline.env` and customize for your stack
3. Run `source .pipeline.env`

## Usage (2 phases)

### Phase 1: Interactive Planning (you drive, ~30 min)
```bash
# Step 1: Requirements
claude
# Type: /plan
# Give your requirements / paste docs
# Review + approve PRD.md → exit

# Step 2: Architecture
claude
# Type: "Read PRD.md. Follow .claude/skills/architect/SKILL.md."
# Review + approve tech-spec.md, shared-contracts.md → exit

# Step 2.5: Test Architecture
claude
# Type: "Read PRD.md, tech-spec.md, shared-contracts.md. Follow .claude/skills/test-architect/SKILL.md."
# Fix any missing infra it reports
# Review + approve test-strategy.md → exit

# Step 3: Task DAG
claude
# Type: "Read PRD.md, tech-spec.md, shared-contracts.md, test-strategy.md. Follow .claude/skills/plan/SKILL.md. Write phase-plan.json."
# Review + approve phase-plan.json → exit
```

### Phase 2: Fully Automatic (walk away)
```bash
source .pipeline.env
./scripts/outer-loop.sh 30 3
# Spawns agents, builds, tests, validates, merges — no human needed
# Stops automatically if circuit breaker trips (30%+ failures)
```

## Customization

Edit `.pipeline.env` for your stack. Ships with C/C++ defaults. See `.pipeline.env.example`.

See `pipeline-v3-complete.md` for full documentation.
