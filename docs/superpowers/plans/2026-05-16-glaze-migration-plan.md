# Glaze Migration Refactoring Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Completely replace `nlohmann/json` with `Glaze` for JSON serialization and deserialization across the Prisma Engine codebase to improve performance and resolve MSVC `/GS` compatibility issues.

**Architecture:** We will replace the internal JSON data structures in `ArchiveJson` to use `glz::json_t` (Glaze's DOM-like structure) or direct object mapping where applicable. For the generic MCP system, we will use `glz::json_t` to maintain flexibility.

**Tech Stack:** C++20, Glaze (v7.5.0+)

---

### Task 1: Refactor ArchiveJson (Resource System)

**Files:**
- Modify: `src/engine/resource/ArchiveJson.h`
- Modify: `src/engine/resource/ArchiveJson.cpp`
- Modify: `src/engine/resource/AssetSerializer.h`

- [ ] **Step 1: Update ArchiveJson.h to use Glaze**
  Replace `nlohmann::json` with `glz::json_t`.

- [ ] **Step 2: Update ArchiveJson.cpp implementation**

- [ ] **Step 3: Update AssetSerializer.h**
  Update `json::parse` and `json.dump` calls to Glaze equivalents (`glz::read_json` and `glz::write_json`).

- [ ] **Step 4: Commit**
```bash
git add src/engine/resource/ArchiveJson.h src/engine/resource/ArchiveJson.cpp src/engine/resource/AssetSerializer.h
git commit -m "refactor(json): Migrate ArchiveJson to Glaze"
```

### Task 2: Refactor MCP System Serialization

**Files:**
- Modify: `src/engine/mcp/serialization/MCPJson.h`
- Modify: `src/engine/mcp/serialization/MCPJson.cpp`
- Modify: `src/engine/mcp/MCPTool.h`

- [ ] **Step 1: Replace nlohmann::json in MCP types**
  Change method signatures to use `glz::json_t`.

- [ ] **Step 2: Update MCPJson implementation**
  Adapt `fromJson` and `toJson` to use Glaze's DOM API.

- [ ] **Step 3: Commit**
```bash
git add src/engine/mcp/serialization/MCPJson.h src/engine/mcp/serialization/MCPJson.cpp src/engine/mcp/MCPTool.h
git commit -m "refactor(mcp): Migrate MCP serialization to Glaze"
```

### Task 4: Cleanup Remaining Includes and Usages

**Files:**
- Modify: `src/editor/core/EditorService.h`
- Modify: `src/editor/core/WebUIEditor.cpp`
- Modify: `src/engine/core/AssetDatabase.h`
- Modify: `src/engine/graphic/Material.cpp`

- [ ] **Step 1: Systematically replace remaining includes**
  Iterate through all files found in research and update them to use Glaze.

- [ ] **Step 2: Final build and test**
  Run `./scripts/build-windows.bat windows-x64-debug` to ensure no nlohmann references remain.

- [ ] **Step 3: Commit**
```bash
git add .
git commit -m "refactor(json): Final cleanup and removal of nlohmann/json dependencies"
```
