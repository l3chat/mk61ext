Import("env")

env.AddCustomTarget(
    name="regression",
    dependencies=None,
    actions=['/bin/bash "$PROJECT_DIR/scripts/run_regression.sh"'],
    title="Regression",
    description="Build and run the host-side calculator regression tests",
)
