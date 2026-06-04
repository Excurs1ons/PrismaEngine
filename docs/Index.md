# Prisma Engine Documentation Index

Welcome to the Prisma Engine documentation. This index provides a complete map of the project's technical documentation.

## 🚀 Getting Started
- [Main README](../README.md) - Project overview and quick start
- [Game Development Guide](GameDevelopmentGuide.md) - Practical guide from real pitfalls (build, loop, input, troubleshooting)
- [Directory Structure](DirectoryStructure.md) - Understanding the codebase organization
- [Requirements](Requirements.md) - System requirements and dependencies

## 🏗️ Architecture & Design
- [Architecture Overview](architecture.md) - High-level system design
- [TODO / Roadmap](TODO.md) - Prioritized task list with 4D decision framework
- [Architecture Optimization](ArchitectureOptimization.md) - Recent core improvements (2024)
- [Implementation Summary](ImplementationSummary.md) - Technical implementation details
- [Path Tracing Plan](plans/2026-05-18-pathtracing-next-steps.md) - Path tracing roadmap (NEE, denoising, ReSTIR)
- [Lifecycle Management](LifecycleManagement.md) - Engine and object lifecycle
- [Lifecycle Management Implementation](LifecycleManagement_Implementation.md) - Deep dive into lifecycle code

## 🎨 Rendering System
- [Rendering Architecture](RenderingSystem.md) - Overview of the rendering pipeline
- [Vulkan Integration](VulkanIntegration.md) - Detailed Vulkan backend for Android/Windows
- [RenderGraph Migration Plan](RenderGraph_Migration_Plan.md) - Future rendering architecture
- [Rendering Redesign](rendering-architecture-redesign.md) - Design rationale for the new renderer
- [Rendering Comparison](rendering-architecture-comparison.md) - Evaluation of different rendering techniques
- [Text Rendering](TextRenderer.md) - Font and text subsystem

## 📦 Resource & Asset Management
- [Resource Manager](ResourceManager.md) - How assets are loaded and managed
- [Asset Serialization](AssetSerialization.md) - Custom asset format details
- [Embedded Resources](EmbeddedResources.md) - Handling internal engine assets

## 📱 Platform Specifics
- [Android Runtime](VulkanIntegration.md) - Android-specific implementation notes
- [Swappy Integration](SwappyIntegration.md) - Frame pacing on Android
- [Device Configuration](DeviceConfiguration.md) - Handling different hardware profiles

## 🔊 Audio & Multimedia
- [Audio System](AudioSystem.md) - XAudio2 and SDL3 audio backends
- [HAP Video System](HAPVideoSystem.md) - High-performance video playback

## 💻 Scripting & UI
- [Scripting System](ScriptingSystem.md) - Engine scripting architecture
- [Scripting Guide](SCRIPTING_GUIDE.md) - How to write scripts for Prisma Engine
- [UI System](UISystem.md) - User interface framework

## 🛠️ Development & Guidelines
- [Coding Style](CodingStyle.md) - Project coding standards
- [Roadmap](Roadmap.md) - Future plans and task list
- [Module Progress](MODULE_PROGRESS.md) - Detailed module status
- [Development MEMO](MEMO.md) - Random notes and troubleshooting
- [Unity Legacy Avoidance](UnityLegacyAvoidance.md) - Design philosophy: avoiding Unity's technical debt (8 rules)

## 🌐 WebUI Editor
- [WebUI Editor](WebUIEditor.md) - Browser-based editor with real-time scene viewport
- [EditorService](PrismaMCP.md#editor--编辑器) - Transport-agnostic editor backend API

## 🤖 AI Integration
- [PrismaMCP](PrismaMCP.md) - Model Context Protocol for AI Agent control (17 tools, 7 categories)
- [MCP Skill](../skills/prisma-mcp/SKILL.md) - AI agent usage skill for Prisma Engine MCP
- [MCP Test Report](MCP_Test_Report.md) - MCP protocol test results and validation (19 tests)

---
*Last Updated: 2026-05-18 (Added Template3D Path Tracing)*
