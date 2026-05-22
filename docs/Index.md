# Prisma Engine Documentation Index

Welcome to the Prisma Engine documentation. This index provides a complete map of the project's technical documentation.

## 🚀 Getting Started
- [Main README](../README.md) - Project overview and quick start
- [Game Development Guide](GameDevelopmentGuide.md) - Practical guide from real pitfalls (build, loop, input, troubleshooting)
- [Directory Structure](DirectoryStructure.md) - Understanding the codebase organization
- [Device Configuration](DeviceConfiguration.md) - System requirements, device profiles and config switches

## 🏗️ Architecture & Design
- [Architecture Overview](Architecture.md) - High-level system design (Driver-Device pattern)
- [Architecture Optimization](ArchitectureOptimization.md) - Recent core improvements and design principles
- [Compute Abstraction Design](ComputeAbstractionDesign.md) - Compute pipeline RHI design
- [Path Tracing](PathTracing.md) - Path tracing system architecture and roadmap
- [Unity Legacy Avoidance](UnityLegacyAvoidance.md) - Design philosophy: avoiding Unity's technical debt (8 rules)

## 🎨 Rendering System
- [Rendering Architecture](RenderingSystem.md) - Overview of the rendering pipeline
- [Vulkan Integration](VulkanIntegration.md) - Detailed Vulkan backend for Android/Windows
- [RenderGraph Migration Plan](RenderGraphMigrationPlan.md) - Future rendering architecture
- [2D Rendering](2DRendering.md) - 2D rendering pipeline design
- [Text Rendering](TextRenderer.md) - Font and text subsystem

## 📦 Resource & Asset Management
- [Resource Manager](ResourceManager.md) - How assets are loaded and managed
- [Asset Serialization](AssetSerialization.md) - Custom asset format details
- [Embedded Resources](EmbeddedResources.md) - Handling internal engine assets

## 📱 Platform Specifics
- [Android Runtime & Vulkan](VulkanIntegration.md) - Android-specific implementation notes
- [Swappy Integration](SwappyIntegration.md) - Frame pacing on Android
- [Device Configuration](DeviceConfiguration.md) - Handling different hardware profiles

## 🔊 Audio
- [Audio System](AudioSystem.md) - XAudio2 and SDL3 audio backends

## 💻 Scripting & UI
- [Scripting System](ScriptingSystem.md) - Engine scripting architecture (CoreCLR C#)
- [Scripting Guide](ScriptingGuide.md) - How to write scripts for Prisma Engine

## 🛠️ Development & Guidelines
- [Roadmap](Roadmap.md) - Future plans, module status, and task list
- [Game Development Guide](GameDevelopmentGuide.md) - Practical development guide and troubleshooting

## 🌐 WebUI Editor
- [WebUI Editor](WebUIEditor.md) - Browser-based editor with real-time scene viewport
- [EditorService](PrismaMCP.md#editor--编辑器) - Transport-agnostic editor backend API

## 🤖 AI Integration
- [PrismaMCP](PrismaMCP.md) - Model Context Protocol for AI Agent control (17 tools, 7 categories)
- [MCP Skill](../skills/prisma-mcp/SKILL.md) - AI agent usage skill for Prisma Engine MCP
- [MCP Test Report](MCPTestReport.md) - MCP protocol test results and validation (19 tests)

## 📂 Archived / Design Docs
- [UI System Design](plans/UISystem.md) - User interface framework (design, implementation pending)
- [RTXGI Integration](plans/RTXGIIntegration.md) - RTXGI integration design
- [HAP Video System](plans/HAPVideoSystem.md) - High-performance video playback (design)
- [C++23/.NET 11/Vulkan Migration](plans/2026-05-21-cpp23-dotnet11-vulkan1351-migration.md) - Planned toolchain upgrade
- [Advanced 2D Rendering](plans/2026-05-16-advanced-2d-rendering-enhancements.md) - Future 2D enhancements

## 📊 Plans Archive
- [Completed Plans](plans/archived/) - Archived implementation plans (CoreCLR scripting, MCP, 2D pipeline, scene serialization)

---
*Last Updated: 2026-05-22 (Documentation restructured)*
