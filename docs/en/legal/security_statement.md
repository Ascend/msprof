# MindStudio Profiler Security Statement

## System Security Hardening

Enable **address space layout randomization** (ASLR) (level 2) in the system. Run the following command to enable it:

    echo 2 > /proc/sys/kernel/randomize_va_space

## User Account Recommendations

 1. Adhere to the principle of least privilege (for example, prohibit write access for `others` and avoid setting file permissions to `666` or `777`).

 2. Before installing or using the tools, ensure the user's `umask` is set to `0027` or a more restrictive value. Otherwise, issues may occur, such as source code compilation failures, configuration file read failures, or excessive permissions for generated directories and files.

 3. All tools in this repository are designed to run with minimal permissions. For security reasons, do not use `root` or other privileged accounts. Always install and execute tools as a regular user.

 4. If a tool depends on CANN, use the CANN package installed by the same non-privileged user. After running the `source` command, do not modify environment variables in `set_env.sh`.

## File Permission Control

 1. Ensure that the directory permission is `750`.

 2. When providing input files to the tool, it is recommended that the file owner matches the process owner of the tool and that file permissions restrict write access for `group` and `others`. By default, tool files written to the drive are not writable by others. You can manually control the permissions for the generated files as needed.

 3. Maintain strict permission control during installation and use. For details about the recommended file permission settings, see the following table.

**File Permission Reference**

| Type                              | Maximum Linux Permission|
| ---------------------------------- | ------------------- |
| Home directory                        | 750 (rwxr-x---)   |
| Program files (including scripts and libraries)  | 550 (r-xr-x---)   |
| Program file directory                      | 550 (r-xr-x---)   |
| Configuration files                          | 640 (rw-r-----)   |
| Configuration file directory                      | 750 (rwxr-x---)   |
| Log files (recorded or archived)  | 440 (r--r-----)   |
| Log files (being recorded)              | 640 (rw-r-----)   |
| Log file directory                      | 750 (rwxr-x---)   |
| Debug files                         | 640 (rw-r-----)   |
| Debug file directory                     | 750 (rwxr-x---)   |
| Temporary file directory                      | 750 (rwxr-x---)   |
| Maintenance and upgrade file directory                  | 770 (rwxrwx---)   |
| Service data files                      | 640 (rw-r-----)   |
| Service data file directory                  | 750 (rwxr-x---)   |
| Key components, private keys, certificates, and ciphertext file directory| 700 (rwx------)   |
| Key components, private keys, certificates, and ciphertext files    | 600 (rw-------)   |
| APIs and scripts for encryption and decryption            | 500 (r-x------)   |

## Vulnerability Security Statement

The MindStudio community highly values the security of community versions. Vulnerability management specialists are specifically designated to handle vulnerability-related matters. To build a more secure AI full-process toolchain, we look forward to your participation.

### Vulnerability Handling Process

For each security vulnerability, the MindStudio community assigns personnel to follow up and handle it. The end-to-end vulnerability handling process is shown in the following figure.

![Vulnerability Handling Process](../figures/vulnerability_handling_process.png)

The following sections explain the vulnerability reporting, assessment, and disclosure processes.

### Vulnerability Reporting

Contact the MindStudio community team by submitting an issue. A security vulnerability specialist will be assigned to contact you as soon as possible.
To ensure security, do not include specific information about security privacy in the issue.

**Response to Reports**

1. The MindStudio community will confirm, analyze, and report security vulnerability issues within three working days, while initiating the security handling process.
2. The MindStudio security team will assign confirmed security vulnerability issues to dedicated personnel and follow up on them.
3. During the process of classifying, confirming, and fixing security vulnerabilities, as well as releasing patches, we will provide timely updates on the report.

### Vulnerability Assessment

The industry widely uses the CVSS standard to assess vulnerability severity. When using CVSS v3.1 for vulnerability assessment, MindStudio sets specific attack scenarios and performs assessments based on the actual impact within those scenarios. Vulnerability severity assessment involves assessing the difficulty of exploitation as well as the impact on confidentiality, integrity, and availability after exploitation, resulting in a numerical score.

#### Vulnerability Assessment Metrics

MindStudio assesses vulnerability severity levels by using the following vector metrics:

- Attack vector (AV): indicates the "remoteness" of an attack and how a vulnerability can be exploited.
- Attack complexity (AC): describes the difficulty of executing an attack and the factors required for a successful attack.
- User interaction (UI): determines whether the attack requires user participation.
- Privileges required (PR): records the level of user authentication required for a successful attack.
- Scope (S): determines whether an attack can affect components with different permission levels.
- Confidentiality (C): measures the impact resulting from information disclosure to unauthorized parties.
- Integrity (I): measures the impact resulting from information tampering.
- Availability (A): measures the impact on users' access to data or services when needed.

#### Assessment Principles

- Assess the severity level of a vulnerability, not the risk.
- The assessment must be based on an attack scenario where a successful attack can compromise the confidentiality, integrity, and availability of the system.
- When a security vulnerability has multiple attack scenarios, use the scenario with the greatest impact (the highest CVSS score) as the basis.
- If a vulnerability exists in an embedded or invoked library, perform the assessment after determining the attack scenario based on how the library is used in the product.
- If a security defect cannot be triggered or does not affect confidentiality, integrity, or availability (CIA), the CVSS score is 0.

#### Assessment Procedure

To assess the severity level of a vulnerability, perform the following steps:

1. Set a possible attack scenario and score based on this attack scenario.
2. Identify the vulnerable component and affected components.
3. Select values for base metrics.

   - Select values for the exploitability metrics (attack vector, attack complexity, privileges required, user interaction, and scope) based on the vulnerable component.

   - Ensure impact metrics (confidentiality, integrity, and availability) reflect the impact on either the vulnerable component or the affected components, whichever is more severe.

#### Severity Rating

| **Severity Rating** | **CVSS Score** | **Vulnerability Fix Time**|
| ------------------------------- | --------------------- | ---------------- |
| Critical               | 9.0 to 10.0             | 7 days             |
| High                     | 7.0 to 8.9              | 14 days            |
| Medium                   | 4.0 to 6.9              | 30 days            |
| Low                      | 0.1 to 3.9              | 30 days            |

### Vulnerability Disclosure

After a security vulnerability is fixed, the MindStudio community will release a security advisory (SA) and a security notice (SN). The SA includes technical details of the vulnerability, type, reporter, CVE ID, affected versions, and fixed versions.
To ensure security for MindStudio users, the MindStudio community will not publicly disclose, discuss, or confirm security issues until after investigation and fixing are complete and an SA has been released.

### Appendixes

#### MindStudio SA

There are currently no security vulnerabilities in the maintained versions.

#### MindStudio SN

The following table describes vulnerabilities involving third-party open-source components.

| CVE ID| Third-Party Component Name| Affected MindStudio Tool/Plugin| Status| Description|
| ------- | -------------- | --------------------------- | ---- | ---- |
|         |                |                             |      |      |

## Data Security Statement

Loading and saving data during the use of the tools may involve data risks.

## Build Security Statement

MindStudio Profiler (msProf) supports building from source. During the build process, the system downloads third-party dependencies, executes shell build scripts, and generates temporary program files and build directories. To reduce security risks, you can perform permission control on files within the source code directory. During the build process, you can modify build scripts as needed to avoid security risks and ensure the security of the build results.

## Running Security Statement

If an exception occurs during operation, the tool will exit the process and print error messages. This is expected behavior. You are advised to locate the specific cause of the error based on the error prompts, such as by viewing log files or result files generated during the collection and parsing process.

## Public API Statement

The msProf project is developed using C++ and Python. The provided external APIs are disclosed in the documentation. Official APIs are only provided as Python APIs. Dynamic libraries do not provide services directly. The exposed APIs are for internal use and user calls are not recommended.

For scripting languages such as Python where source code is released, use the public APIs specified in the documentation. Do not directly call source code APIs that are not explicitly disclosed.

## Usage of Safe Functions

While unsafe functions are not forcibly disabled, you are advised to use their safe variants that explicitly take a buffer length parameter, for example, `memset_s` and `memcpy_s`.
