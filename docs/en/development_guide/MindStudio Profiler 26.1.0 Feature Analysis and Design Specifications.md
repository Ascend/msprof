
# Background

Building on the end-to-end software and hardware scheduling capabilities of the Ascend 950PR/Ascend 950DT chips, these features primarily extend tuning capabilities for the scheduler, computation and communication paths, AICore computing units, and system hardware metrics on Ascend 950PR/Ascend 950DT, thereby supporting performance tuning and analysis.

# Use Cases

---

Feature 1: Presenting D/U-Die bandwidth information

Usage restriction: Ascend 950PR/Ascend 950DT

Usage constraint: NA

DFX design:

- Compatibility: This is a new feature for this chip generation and does not involve compatibility issues.
- Maintainability: NA
- Reliability: The Stars data reporting task includes link IDs and data correctness checks.

---

Feature 2: Presenting fusion tasks

Usage restriction: Ascend 950PR/Ascend 950DT

Usage constraint: NA

DFX design:

- Compatibility: This is a new feature for this chip generation and does not involve compatibility issues.
- Maintainability: NA
- Reliability: A fusion task includes subtask types such as AIC, AICPU, and CCU. Add subtask range checks.

---

Feature 3: Statistics and presentation of DPU operator execution time

Usage restriction: Ascend 950PR/Ascend 950DT and DPU-based computation

Usage constraint: NA

DFX design:

- Compatibility: This is a new feature for this chip generation and does not involve compatibility issues.
- Maintainability: NA
- Reliability: Verify the correctness of DPU task data parsing.

---

Feature 4: Inheriting features across chip generations

Usage restriction: Ascend 950PR/Ascend 950DT chips

Usage constraint: NA

DFX design:

- Compatibility: Align parameter and feature behavior between chip generations to ensure backward compatibility.
- Maintainability: NA
- Reliability: Verify the validity of command-line parameters and parameter configurations.

# Design Details

1. Design approach

   ```text
   Ascend 950PR/Ascend 950DT all-scenario architecture diagram

     ┌───────────────────────────────────────────────────────────────────────────────┐
     │                                 AI Framework                                  │
     │  ┌─────────────────────────────────────────────────────────────────────────┐  │
     │  │                                 PyTorch                                 │  │
     │  └─────────────────────────────────────────────────────────────────────────┘  │
     └───────────────────────────────────────────────────────────────────────────────┘
     ┌───────────────────────────────────────────────────────────────────────────────┐
     │                                   CANN                                        │
     │  ┌─────────────────────────────────────────────────────────────────────────┐  │
     │  │                               AscendCL                                  │  │
     │  └─────────────────────────────────────────────────────────────────────────┘  │
     │  ┌───────────────────────┐    ┌───────────────────┐     ┌──────────────────┐  │
     │  │           GE          │    │        ACLNN      │     │       ...        │  │
     │  └───────────────────────┘    └───────────────────┘     └──────────────────┘  │
     │  ┌─────────────────────────────────────────────────────────────────────────┐  │
     │  │                                 Runtime                                 │  │
     │  └─────────────────────────────────────────────────────────────────────────┘  │
     └───────────────────────────────────────────────────────────────────────────────┘
     ┌───────────────────────────────────────────────────────────────────────────────┐
     │                                  NPU                                          │
     │  ┌──────────────┐    ┌──────────────┐   ┌──────────────┐   ┌───────────────┐  │
     │  │              │    │              │   │     AICPU    │   │               │  │
     │  │      LP      │    │              │   └──────────────┘   │               │  │
     │  │              │    │              │   ┌──────────────┐   │               │  │
     │  └──────────────┘    │              │   │      CCU     │   │               │  │
     │  ┌──────────────┐    │     Stars    │   └──────────────┘   │      UB       │  │
     │  │              │    │              │   ┌──────────────┐   │               │  │
     │  │   L2 Cache   │    │              │   │    AICore    │   │               │  │
     │  │              │    │              │   └──────────────┘   │               │  │
     │  └──────────────┘    └──────────────┘                      └───────────────┘  │
     └───────────────────────────────────────────────────────────────────────────────┘

   ```

   Building on the end-to-end software and hardware scheduling capabilities of the Ascend 950PR/Ascend 950DT chips and leveraging their capabilities, these features primarily extend tuning capabilities for the scheduler, computation and communication paths, AICore computing units, and system-level hardware metrics.

   - Add the fusion task type to the Stars scheduler.
   - Support inter-die bandwidth for AICore computing units.
   - Support DPU operator scheduling and execution.
   - Present CCU communication instructions and bandwidth.

2. Implementation design

   1. D/U-Die bandwidth implementation

      ```text
        D/U-Die structure

        ┌───────┐    ┌───────┐    ┌───────┐    ┌───────┐
        │ UDie0 │<-->│ DDie0 │<-->│ DDie1 │<-->│ UDie1 │
        └───────┘    └───────┘    └───────┘    └───────┘
      ```

      Inter-die bandwidth data is reported in Stars data format. After the profile data is written to drive and parsed, a timeline bar chart is generated to support analysis of bandwidth between D/U-Dies. The bandwidth scope is shown in the D/U-Die structure above, including bandwidth statistics for UDie0-DDie0, DDie0-DDie1, and DDie1-UDie1.

      DFX design

      - Performance: Add a type of hardware profile data reporting. The additional processing can overlap with concurrent processing during data parsing, with no performance impact.
      - Reliability: The Stars data reporting task includes link IDs and data correctness checks.

   2. Fusion task presentation
      Profile data is collected through Profiling. Fusion tasks are reported in Stars data format. After the profile data is written to drive and parsed, task data is generated and subtask information of fusion tasks is presented in the timeline trace.

      A fusion task includes the following types of subtasks:
      - AIC/AIV
      - AICPU
      - CCU

      DFX design
      - Performance: Add a new data type and distribute the data through Stars. The number of fusion tasks is small, so there is almost no performance impact.
      - Reliability: Add subtask range checks to ensure that newly added subtasks can be detected in a timely manner.
      - Security: The data format is fixed and contains no variable-length data. Ensure data format integrity.

   3. DPU operator implementation

      ```text
      DPU data presentation


                                       ┌─────────┐
      CANN: CPU                        │ runtime │─────────────┐
                                       └────│────┘             │
                 Thread ID ─────────────────│──────────────────│──────────────────────────
                                            │             ┌────V────┐
      CANN: CPU                             │             │   DPU   │
                                            │             └─────────┘
                 Stream ID ─────────────────│─────────────────────────────────────────────
                                            │
      Ascend Hardware: NPU                  │      ┌─────────┐
                                            └──────│> NPU    │
                                                   └─────────┘
                 Stream ID ───────────────────────────────────────────────────────────────

      ```

      Ascend 950PR/Ascend 950DT chips have a DPU compute module on the host side. The module includes computation and communication tasks and supports differentiation by device ID. DPU and NPU IDs are separate. Data is also associated and presented through unique IDs. A new CANN: DPU track is added to the timeline trace to support performance analysis.

      **Computation tasks**: Tasks are dispatched to and executed directly by the DPU module on the host side and are not dispatched to the device.

      **Communication tasks**: Tasks are dispatched to the DPU from the host and are also dispatched to the NPU side for data communication through message exchanges.

      DFX design

      - Performance: DPU data can be parsed concurrently, which can overlap with other processing, resulting in almost no performance impact.
      - Reliability: Verify data matching and data loss for the new DPU module. The unique ID currently ensures correct data association.

   4. Command-line parameter implementation for chips of this generation
      - Complete the `reports` parameter: Align with this chip generation by supporting the `reports` parameter to filter the tracks to be presented in the timeline trace.
      - Default DB export: Complete the default DB file export during data parsing through msProf.
      - Copy profile data deliverables (`task_time.csv` and `soc_pmu.csv`) to the `ASCEND_PROFILER_OUTPUT` directory to support data presentation and analysis.

      DFX design
      - Performance: Inheriting existing features does not affect performance.
      - Reliability: NA

# User Guide

- Interface description: Ascend 950PR/Ascend 950DT supports the `reports` parameter to control the timeline content to be returned.
- Interface prototype: `msprof --reports`
- Input/Output parameters

  | Parameter | Input/Output | Type      | Description                                 | Value Range |
  | --------- | ------------ | --------- | ------------------------------------------- | ----------- |
  | `reports` | Input        | JSON file | Controls the content of the timeline trace. | NA          |

- Return values

  | Parameter | Type | Description | Value Range |
  | --------- | ---- | ----------- | ----------- |
  |           |      |             |             |

- Exception handling:

  - The JSON file size or permissions are invalid.

  - The JSON content is invalid. Check the content and display an error message.

- Constraints: NA

- Change description: NA

- Example:

  `msprof --export=on --output=/home/profiler_data/PROF_XXX --reports=${INSTALL_DIR}/tools/profiler/profiler_tool/analysis/msconfig/reports_sample_config.json`

# Test Plan

| Test Case | Prerequisites | Expected Input | Expected Output |
| --- | --- | --- | --- |
| D/U-Die bandwidth data collection | The Ascend 950PR/Ascend 950DT CANN software package is correctly installed. | Run the service application through msprof: `msprof --output=* ./main [args]` | 1. PROF profile data is collected, and a message indicating that automatic parsing is complete is displayed.<br>2. Timeline data is generated, and a new track displays a D/U-Die bandwidth bar chart. |
| Fusion task presentation | The Ascend 950PR/Ascend 950DT CANN software package is correctly installed. | Run the service application through msprof: `msprof --output=* ./main [args]` | 1. PROF profile data is collected, and a message indicating that automatic parsing is complete is displayed.<br>2. Timeline data is generated, and fusion task execution-time information is displayed under Ascend Hardware. |
| DPU operator execution-time statistics and presentation | The Ascend 950PR/Ascend 950DT CANN software package is correctly installed. | Run the service application through msprof: `msprof --output=* ./main [args]` | 1. PROF profile data is collected, and a message indicating that automatic parsing is complete is displayed.<br>2. Timeline data is generated, and DPU task execution-time information is displayed under CANN: DPU. |
| msprof command-line support for the `reports` parameter | The Ascend 950PR/Ascend 950DT CANN software package is correctly installed.<br>`reports_sample_config.json` is included and only `"ascend": true` is configured; all other options are set to `false`. | Run the service application through msprof: `msprof --output=* ./main [args] --reports="reports_sample_config.json"` | 1. PROF profile data is collected, and a message indicating that automatic parsing is complete is displayed.<br>2. Timeline data is generated, and only the Ascend Hardware track is presented. |
| msprof command-line support for parsing the default exported DB | The Ascend 950PR/Ascend 950DT CANN software package is correctly installed. | Run the service application through msprof: `msprof --output=* ./main [args]` | 1. PROF profile data is collected, and a message indicating that automatic parsing is complete is displayed.<br>2. The generated deliverables include a DB file. |
| | | | |

# (Optional) Drawbacks

DPU association correctness: Operator tasks are dispatched through the CANN platform, and tasks on the NPU and DPU sides are mixed, making correct data association challenging.

Complexity: All-scenario performance data collection and presentation across heterogeneous systems and AI frameworks depend heavily on component data and increase the difficulty of understanding the data.

# (Optional) Alternatives

No alternatives are currently available.

# (Optional) Unresolved Questions

No unresolved questions are currently involved.
