#pragma once

// ============================================================
// PrismaEngine SDK — Unified Public API Header
// ============================================================
// Include this single header to access the full PrismaEngine API.
// Usage: #include <PrismaEngine/PrismaEngine.h>
// ============================================================

// ── Build Config & Export Macros ──
#include "Build.h"
#include "Export.h"
#include "Common.h"

// ── Application Framework ──
#include "app/Application.h"
#include "app/Engine.h"
#include "app/EngineLauncher.h"
#include "app/ProjectConfig.h"
#include "EntryPoint.h"

// ── Core ECS ──
#include "core/ECS.h"
#include "core/Component.h"
#include "core/EntityManager.h"
#include "core/Event.h"
#include "core/Handle.h"
#include "core/ISubSystem.h"
#include "core/IWorld.h"
#include "core/Layer.h"
#include "core/LayerStack.h"
#include "core/Node.h"
#include "core/Singleton.h"
#include "core/Systems.h"
#include "core/Timestep.h"
#include "core/UUID.h"
#include "core/StringHash.h"

// ── Asset System ──
#include "core/Asset.h"
#include "core/AssetManager.h"
#include "core/AssetDatabase.h"

// ── Math ──
#include "math/MathTypes.h"
#include "math/MurmurHash3.h"

// ── Graphics ──
#include "graphic/RenderSystem.h"
#include "graphic/Renderer.h"
#include "graphic/Renderer2D.h"
#include "graphic/Mesh.h"
#include "graphic/Material.h"
#include "graphic/Shader.h"
#include "graphic/ICamera.h"
#include "graphic/OrthographicCamera.h"
#include "graphic/PerspectiveCamera.h"
#include "graphic/TextureAtlas.h"
#include "graphic/RenderPass.h"
#include "graphic/RenderGraph.h"
#include "graphic/MeshRenderer.h"
#include "graphic/PrimitiveComponent.h"
#include "graphic/FrustumCulling.h"
#include "graphic/RenderComponent.h"
#include "graphic/RenderDesc.h"
#include "graphic/interfaces/Interfaces.h"

// ── Input ──
#include "input/Input.h"
#include "input/InputManager.h"
#include "input/KeyCode.h"
#include "input/EnhancedInputManager.h"

// ── Audio ──
#include "audio/IAudioDevice.h"
#include "audio/AudioTypes.h"

// ── Logger ──
#include "logger/Logger.h"
#include "logger/LogEntry.h"
#include "logger/LogScope.h"

// ── Platform ──
#include "platform/Platform.h"
#include "platform/DynamicLoader.h"

// ── Window ──
#include "window/Window.h"

// ── Transform ──
#include "transform/Transform.h"
#include "transform/ITransform.h"
#include "transform/Camera.h"
#include "transform/FPSCounter.h"

// ── Scene ──
#include "scene/Scene.h"
#include "scene/SceneManager.h"
#include "scene/SceneNode.h"

// ── Resource ──
#include "resource/TextureAsset.h"
#include "resource/MeshAsset.h"
#include "resource/Archive.h"
#include "resource/AssetSerializer.h"

// ── Physics ──
#include "physics/CollisionSystem.h"
#include "physics/PhysicsComponents.h"
#include "physics/PhysicsSystem.h"

// ── Threading ──
#include "threading/JobSystem.h"
#include "threading/ThreadManager.h"

// ── UI (2D) ──
#include "ui/UIComponent.h"
#include "ui/2d/CanvasComponent.h"
#include "ui/2d/ButtonComponent.h"

// ── Serialization ──
#include "serialization/Serializable.h"
#include "serialization/ScriptComponent.h"

// ── Utils ──
#include "utils/StringUtils.h"
#include "utils/ImageUtils.h"

// ── Scripting C# ──
#include "scripting/CoreCLRHost.h"
#include "scripting/ScriptEngine.h"
#include "scripting/SRPGraphicsAPI.h"
