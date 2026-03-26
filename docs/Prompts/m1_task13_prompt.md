# Alpha Milestone 1 Task 13 Prompt

Implement Milestone 1, Task 13: deterministic regression coverage, save/load equivalence coverage, and real Milestone 1 validation entrypoints.

Before writing code, read these docs and treat them as binding:
- `docs/alpha_tech_stack_handoff.md`
- `docs/milestone_plan.md`
- `docs/simulation_api.md`
- `docs/save_format.md`
- `docs/coding_standards.md`
- `README.md`

Use them proactively while implementing:
- `docs/milestone_plan.md` defines the Milestone 1 exit criteria for deterministic 12-month replay, save/load equivalence, and prototype performance/debug validation
- `docs/save_format.md` defines the persistence boundary that must survive equivalence testing
- `docs/coding_standards.md` is the determinism and test-discipline guardrail
- `docs/alpha_tech_stack_handoff.md` defines the macOS prototype requirement
- `README.md` documents the repo-level validation entrypoints that should become real Milestone 1 workflows

Objective:
Create the final Milestone 1 hardening pass:
- add deterministic regression coverage for multi-month runs and fixed command streams
- add save/load equivalence coverage for continuing future results
- turn the placeholder repo validation scripts into real Milestone 1 build/test/export entrypoints
- close the milestone against the documented acceptance criteria

Scope:
- add deterministic tests for the native sim over multi-month runs
- add save/load equivalence tests covering interruption mid-run and continued future behavior
- add targeted scenario/regression fixtures for zoning, roads, and blocked projects
- update the repo scripts and CI wiring so they exercise real Milestone 1 validation rather than only placeholder checks
- add a practical profiling/debug validation path for month timings in the prototype

Required behavior:
- a fixed seed plus fixed commands produces identical 12-month results across repeated runs
- save, quit, reload, and continue produces the same future results as uninterrupted execution
- regression coverage exists for at least:
  - zoning updates
  - road drawing / road completion effects
  - blocked or slowed project reporting
  - settlement summary/detail stability across repeated runs
- repo validation entrypoints execute real Milestone 1 work:
  - native build
  - automated tests
  - macOS export or launch validation as appropriate for the current toolchain
- CI is updated so the new tests can run in an automated path where practical

Implementation guidance:
- prefer compact golden or fixture-based tests over brittle huge snapshots
- keep deterministic expectations focused on authoritative outputs, not presentation-only noise
- if full export automation is environment-sensitive, still replace placeholder scripts with the real canonical steps and document prerequisites clearly
- make profiling/debug checks useful for catching regressions rather than decorative

Explicit non-goals for this task:
- no new gameplay systems beyond what earlier Milestone 1 tasks introduced
- no final polish work
- no Milestone 2 content

Constraints:
- keep tests and validation deterministic
- preserve the repo/module homes described in `docs/milestone_plan.md`
- keep authoritative checks centered on sim-owned outputs
- keep the implementation intentionally small, clean, and maintainable

Suggested first files:
- `sim/tests/unit/`
- `sim/tests/scenario/`
- `sim/tests/regression/`
- `data/scenarios/test/`
- `tools/scripts/run_tests.sh`
- `tools/scripts/build_macos.sh`
- `tools/scripts/export_macos.sh`
- `ci/github/workflows/validate.yml`
- updates to `README.md` if workflow expectations change materially

Acceptance criteria:
- the Milestone 1 test suite includes deterministic multi-month replay coverage
- the Milestone 1 test suite includes save/load equivalence coverage
- repo validation scripts perform real build/test/export or launch validation rather than placeholder-only checks
- CI is aligned with the real Milestone 1 validation flow
- the resulting repository satisfies the documented Milestone 1 acceptance criteria in practical form
- the implementation is consistent with:
  - `docs/milestone_plan.md`
  - `docs/save_format.md`
  - `docs/simulation_api.md`
  - `docs/coding_standards.md`
  - `docs/alpha_tech_stack_handoff.md`

Validation requirements:
- before finalizing, run validation for the full Milestone 1 codebase, not just the newly added files
- if any previously working Milestone 1 behavior regresses while doing this task, fix it as part of the task rather than treating it as out of scope
- validate the cumulative build, test, save/load, launch, and export surface that exists by the end of Milestone 1 and report exactly what was run

Recommended tests:
- fixed-seed 12-month golden run with a stable command script
- save/load equivalence test that saves after several months, reloads, and compares future results
- regression test for zoning updates affecting settlement/project state deterministically
- regression test for roads affecting access/movement outcomes deterministically
- regression test for blocker reporting stability
- build/export smoke test for the macOS prototype path where practical

Final response requirements:
- summarize exactly what was added
- list any deviations from the Milestone 1 acceptance criteria or validation assumptions
- identify any ambiguities or conflicts found in the docs
- state what cumulative validation was run for all Milestone 1 code through this task
- state whether Milestone 1 is fully complete after this task or what concrete gap remains
