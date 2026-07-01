# Joint CSV Trajectory Generation Design

## Status

Approved design from `superpowers:brainstorming`.

This document covers the first implementation slice for generating PQKit trajectory data from a CSV of machining robot joints and gantry guide-axis values.

## Goal

Add a Qt UI feature that reads a CSV file containing 6-axis machining robot joint values plus 3-axis gantry guide values, generates trajectory data in PQKit with `PQAPIAddAbsJointPath`, and exports a verification CSV.

The user-facing output is:

- A generated PQKit trajectory.
- A verification CSV under `csvData/`.

## Non-Goals

- Do not implement an independent forward-kinematics solver.
- Do not add automatic robot filtering by axis count in the first version.
- Do not support separate selection of `R1`, `R2`, and `R3` objects.
- Do not implement a custom logging window.
- Do not resolve every PQKit edge case before implementation; keep those as explicit verification items.

## UI Placement

Add the feature under the existing MainWindow ribbon category:

- Category: programming and simulation.
- Panel: trajectory planning.
- Action label meaning: "generate trajectory from joint CSV".

The implementation should follow the existing project encoding and UI text conventions when adding the actual Chinese label.

## UI Inputs

The dialog should collect:

- CSV file path.
- New trajectory name.
- Machining robot: selected from `PQ_ROBOT` objects.
- Positioning guide: selected from `PQ_ROBOT` objects.
- Rated velocity: used when CSV `pathVel` is missing or invalid.
- Speed percent: applied to all generated points.
- Approach: applied to all generated points.
- Verification pose format:
  - Quaternion.
  - Euler angles.

The machining robot and positioning guide are two different PQKit robot objects. The positioning guide represents the 3-axis gantry. The UI must not ask the user to select three separate external-axis objects.

## CSV Format

Required columns:

```text
J1_deg,J2_deg,J3_deg,J4_deg,J5_deg,J6_deg,R1_pos_mm,R2_pos_mm,R3_pos_mm
```

Optional columns:

```text
index,time,pathVel
```

Parsing rules:

- Resolve columns by header name, not fixed index.
- Convert `J1_deg..J6_deg` from degrees to radians before calling PQKit.
- Pass `R1_pos_mm..R3_pos_mm` unchanged as guide-axis values.
- Use `pathVel` as the row velocity when it is present and numeric.
- Use the UI rated velocity when `pathVel` is absent or invalid.
- Skip rows with missing, empty, or non-numeric required values.
- Report total rows, valid rows, and skipped rows at the end.

## Precheck Rules

Before calling PQKit, validate:

- CSV path exists and is readable.
- Trajectory name is not empty.
- Machining robot is selected.
- Positioning guide is selected.
- Machining robot and positioning guide are not the same object.
- Required CSV columns exist.
- At least one valid CSV row exists.
- Rated velocity, speed percent, and approach are valid numeric values.
- PQKit pointer is available.

Axis-count checks are advisory:

- If machining robot axis count is not 6, warn and allow the user to continue or cancel.
- If positioning guide axis count is not 3, warn and allow the user to continue or cancel.

## PQKit API Mapping

Primary API:

```cpp
PQAPIAddAbsJointPath(
    ULONG i_ulRobotID,
    DOUBLE* i_dRobotJoints,
    INT i_nRJointsCount,
    DOUBLE* i_dGuideJoints,
    INT i_nGJointsCount,
    DOUBLE* i_dPositionerJoints,
    INT i_nPJointsCount,
    DOUBLE* i_dVelocity,
    DOUBLE* i_dSpeedPercent,
    INT* i_nApproach,
    INT i_nPointCount,
    ULONG i_uPathID,
    ULONG* o_uPathID
)
```

Parameter mapping:

- `i_ulRobotID`: selected machining robot ID.
- `i_dRobotJoints`: contiguous array of valid point robot joints, size `pointCount * 6`.
- `i_nRJointsCount`: `6`.
- `i_dGuideJoints`: contiguous array of guide values, size `pointCount * 3`.
- `i_nGJointsCount`: `3`.
- `i_dPositionerJoints`: `nullptr` or an empty array.
- `i_nPJointsCount`: `0`.
- `i_dVelocity`: size `pointCount`.
- `i_dSpeedPercent`: size `pointCount`, each value copied from the UI input.
- `i_nApproach`: size `pointCount`, each value copied from the UI input.
- `i_nPointCount`: valid CSV row count.
- `i_uPathID`: first implementation tries `0`.
- `o_uPathID`: generated trajectory ID.

Important assumptions:

- The 3-axis gantry is a guide, so `R1/R2/R3` belong in `i_dGuideJoints`.
- Positioner values are not used in this feature.
- `i_uPathID = 0` might mean no insertion reference path. This must be tested.

## Result Handling

On API failure:

- Show a concise error message.
- Include `HRESULT`, machining robot ID/name, valid point count, and `i_uPathID`.
- Do not write a success verification CSV.

On API success:

- Treat `o_uPathID` as the generated trajectory ID.
- Attempt to read back generated trajectory points and poses.
- Export verification CSV to:

```text
csvData/<trajectory-name>_fk_verify.csv
```

If the path already exists, append `_1`, `_2`, and so on.

Verification CSV should include:

- Optional original metadata if present: `index`, `time`, `pathVel`.
- Original input values: `J1_deg..J6_deg`, `R1_pos_mm..R3_pos_mm`.
- PQKit pose values: `x`, `y`, `z`.
- Pose orientation columns in the selected format:
  - Quaternion, or
  - Euler angles.
- Per-row status if pose readback fails for individual points.

Individual point readback failures should not fail the whole export.

## User Feedback

Precheck failures:

- Do not call PQKit.
- Show the exact missing or invalid condition.

PQKit failures:

- Show a failure summary with the key call context.

Success:

- Show generated trajectory ID.
- Show trajectory name.
- Show CSV total row count.
- Show valid point count.
- Show skipped row count.
- Show verification CSV path.
- Show pose readback failure count if any.

## Open Risks And Verification Items

These must be tested during implementation:

- Whether `i_uPathID = 0` is accepted by `PQAPIAddAbsJointPath`.
- Whether `PQAPIAddAbsJointPath` generates one trajectory or multiple AbsJ trajectories when `pointCount > 1`.
- How to set or rename the generated trajectory to the user-provided trajectory name.
- Which existing PQKit APIs should be used to read generated trajectory point IDs.
- Which existing PQKit APIs should be used to read point poses in quaternion and Euler forms.
- Whether returned pose memory uses `PQAPIFree`, `PQAPIFreeArray`, or another release method.

## Expected Code Areas

Likely additions:

- A new Qt dialog for the joint CSV trajectory feature.
- A non-UI generator/service class for CSV parsing, prechecks, PQKit calls, and verification CSV export.

Likely modifications:

- `include/MainWindow.h`
- `src/MainWindow.cpp`
- `robot2022.vcxproj`
- `robot2022.vcxproj.filters`
- Potentially resource or UI project entries if `.ui` files are used.

