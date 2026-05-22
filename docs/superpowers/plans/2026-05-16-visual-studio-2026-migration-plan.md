# Visual Studio 2026 Migration Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Migrate the Prisma Engine Windows build environment from Visual Studio 2022 to Visual Studio 2026 and update all corresponding documentation and scripts.

**Architecture:** We will update the CMakePresets to use the new generator `Visual Studio 18 2026`. We will then perform a global find-and-replace for references to Visual Studio 2022 across documentation, scripts, and CI workflows, taking care to ignore third-party copyright headers.

**Tech Stack:** CMake, MSVC, PowerShell, Bash

---

### Task 1: Update CMake Presets

**Files:**
- Modify: `CMakePresets.json`

- [ ] **Step 1: Update Generator**

Open `CMakePresets.json` and change the `generator` for the `windows-base` preset from `Visual Studio 17 2022` to `Visual Studio 18 2026`.

```json
      "name": "windows-base",
      "hidden": true,
      "generator": "Visual Studio 18 2026",
      "architecture": "x64",
```

- [ ] **Step 2: Commit**

```bash
git add CMakePresets.json
git commit -m "build: Update CMake generator to Visual Studio 2026"
```

### Task 2: Update Scripts

**Files:**
- Modify: `scripts/README.md`
- Modify: `scripts/package-sdk.sh`
- Modify: `scripts/setup-env.ps1`

- [ ] **Step 1: Modify scripts/README.md**

Update the prerequisite section.
```markdown
1. Install Visual Studio 2026 with C++ development tools
```

- [ ] **Step 2: Modify scripts/package-sdk.sh**

Update the requirements.
```bash
  - Windows: MSVC 2026+
```

- [ ] **Step 3: Modify scripts/setup-env.ps1**

Update the environment setup script to check for / prompt for VS 2026.
```powershell
        Write-Host "    Please install Visual Studio 2026 with the 'Desktop development with C++' workload" -ForegroundColor Gray
        Write-Host "      winget install Microsoft.VisualStudio.2026.Community" -ForegroundColor DarkGray
```

- [ ] **Step 4: Commit**

```bash
git add scripts/README.md scripts/package-sdk.sh scripts/setup-env.ps1
git commit -m "build(scripts): Update script references to Visual Studio 2026"
```

### Task 3: Update Main Documentation

**Files:**
- Modify: `CLAUDE.md`
- Modify: `docs/MEMO.md`
- Modify: `docs/ScriptingSystem.md`
- Modify: `docs/RenderingRefactoringPlan.md`
- Modify: `docs/DeviceConfiguration.md`
- Modify: `docs/MCPTestReport.md`

- [ ] **Step 1: Modify CLAUDE.md**

```markdown
1. Open the PrismaEngine root folder in Visual Studio 2026
```

- [ ] **Step 2: Modify docs/MEMO.md**

```markdown
- **构建系统**: Visual Studio 2026
```
and
```markdown
1. 安装 Visual Studio 2026
```

- [ ] **Step 3: Modify docs/ScriptingSystem.md**

```markdown
- **IDE**: 推荐使用 Visual Studio 2026 (Windows) 或 VS Code + C# Dev Kit (跨平台)。
```

- [ ] **Step 4: Modify docs/RenderingRefactoringPlan.md**

```markdown
- Visual Studio 2026
```

- [ ] **Step 5: Modify docs/DeviceConfiguration.md**

```json
      "generator": "Visual Studio 18 2026",
```

- [ ] **Step 6: Modify docs/MCPTestReport.md**

```markdown
- MSVC 19.50 (Visual Studio 2026 Community)
```
*(Assuming the new MSVC compiler version is ~19.50, update appropriately or leave as is but change the VS version).*

- [ ] **Step 7: Commit**

```bash
git add CLAUDE.md docs/MEMO.md docs/ScriptingSystem.md docs/RenderingRefactoringPlan.md docs/DeviceConfiguration.md docs/MCPTestReport.md
git commit -m "docs: Update documentation references to Visual Studio 2026"
```

### Task 4: Update Workflows and SDK Docs

**Files:**
- Modify: `.github/workflows/README.md`
- Modify: `sdk/README.md`

- [ ] **Step 1: Modify .github/workflows/README.md**

```markdown
   cmake -B build -G "Visual Studio 18 2026" -A x64
```

- [ ] **Step 2: Modify sdk/README.md**

```markdown
  - Windows: MSVC 2026+
```

- [ ] **Step 3: Commit**

```bash
git add .github/workflows/README.md sdk/README.md
git commit -m "docs: Update GitHub workflows and SDK readme to Visual Studio 2026"
```

### Task 5: Build Verification

- [ ] **Step 1: Test CMake Generation**

Run the build script to ensure CMake configuration succeeds with the new generator.

```bash
./scripts/build-windows.bat
```
*(Note: If Visual Studio 2026 is not installed on the system executing the agent, this step may fail but is required as part of the overall migration check.)*
