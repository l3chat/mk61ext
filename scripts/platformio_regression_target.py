Import("env")

env.AddCustomTarget(
    name="regression",
    dependencies=None,
    actions=['/bin/bash "$PROJECT_DIR/scripts/run_regression.sh"'],
    title="Regression",
    description="Build and run the host-side calculator regression tests",
)

env.AddCustomTarget(
    name="trace",
    dependencies=None,
    actions=['/bin/bash "$PROJECT_DIR/scripts/trace_sequence.sh"'],
    title="Trace",
    description="Build and run the host-side calculator trace tool (use TRACE_SEQ to pass a sequence)",
)
