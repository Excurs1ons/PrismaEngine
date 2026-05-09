# Graph Report - .  (2026-05-09)

## Corpus Check
- Large corpus: 391 files · ~137,010 words. Semantic extraction will be expensive (many Claude tokens). Consider running on a subfolder, or use --no-semantic to run AST-only.

## Summary
- 2122 nodes · 2432 edges · 86 communities detected
- Extraction: 96% EXTRACTED · 4% INFERRED · 0% AMBIGUOUS · INFERRED: 87 edges (avg confidence: 0.8)
- Token cost: 0 input · 0 output

## Community Hubs (Navigation)
- [[_COMMUNITY_Core Math & Types|Core Math & Types]]
- [[_COMMUNITY_Platform Abstraction|Platform Abstraction]]
- [[_COMMUNITY_SDL3 Audio Device|SDL3 Audio Device]]
- [[_COMMUNITY_PacMan Game Logic|PacMan Game Logic]]
- [[_COMMUNITY_Asset Serialization|Asset Serialization]]
- [[_COMMUNITY_Null Audio Device|Null Audio Device]]
- [[_COMMUNITY_Audio Synthesis|Audio Synthesis]]
- [[_COMMUNITY_Audio Device Abstraction|Audio Device Abstraction]]
- [[_COMMUNITY_Graphic Resource Pipeline|Graphic Resource Pipeline]]
- [[_COMMUNITY_Vulkan Resource Factory|Vulkan Resource Factory]]
- [[_COMMUNITY_Mono Scripting Runtime|Mono Scripting Runtime]]
- [[_COMMUNITY_Render Commands|Render Commands]]
- [[_COMMUNITY_Vulkan Pipeline State|Vulkan Pipeline State]]
- [[_COMMUNITY_Logging System|Logging System]]
- [[_COMMUNITY_Input System|Input System]]
- [[_COMMUNITY_PacMan Game Controller|PacMan Game Controller]]
- [[_COMMUNITY_UI Button Component|UI Button Component]]
- [[_COMMUNITY_Vulkan Shader Compilation|Vulkan Shader Compilation]]
- [[_COMMUNITY_Shader Build Tools|Shader Build Tools]]
- [[_COMMUNITY_Sprite Renderer|Sprite Renderer]]
- [[_COMMUNITY_PacMan Game Board|PacMan Game Board]]
- [[_COMMUNITY_Android Native Bridge|Android Native Bridge]]
- [[_COMMUNITY_ECS Component System|ECS Component System]]
- [[_COMMUNITY_Audio Driver Abstraction|Audio Driver Abstraction]]
- [[_COMMUNITY_Engine Core Initialization|Engine Core Initialization]]
- [[_COMMUNITY_Platform Context|Platform Context]]
- [[_COMMUNITY_Block Game World|Block Game World]]
- [[_COMMUNITY_Camera System|Camera System]]
- [[_COMMUNITY_Entity Transform System|Entity Transform System]]
- [[_COMMUNITY_Runtime Entry Point|Runtime Entry Point]]
- [[_COMMUNITY_Community 31|Community 31]]
- [[_COMMUNITY_Community 32|Community 32]]
- [[_COMMUNITY_Community 33|Community 33]]
- [[_COMMUNITY_Community 34|Community 34]]
- [[_COMMUNITY_Community 35|Community 35]]
- [[_COMMUNITY_Community 36|Community 36]]
- [[_COMMUNITY_Community 37|Community 37]]
- [[_COMMUNITY_Community 38|Community 38]]
- [[_COMMUNITY_Community 39|Community 39]]
- [[_COMMUNITY_Community 40|Community 40]]
- [[_COMMUNITY_Community 41|Community 41]]
- [[_COMMUNITY_Community 42|Community 42]]
- [[_COMMUNITY_Community 43|Community 43]]
- [[_COMMUNITY_Community 44|Community 44]]
- [[_COMMUNITY_Community 45|Community 45]]
- [[_COMMUNITY_Community 46|Community 46]]
- [[_COMMUNITY_Community 47|Community 47]]
- [[_COMMUNITY_Community 48|Community 48]]
- [[_COMMUNITY_Community 49|Community 49]]
- [[_COMMUNITY_Community 50|Community 50]]
- [[_COMMUNITY_Community 52|Community 52]]
- [[_COMMUNITY_Community 53|Community 53]]
- [[_COMMUNITY_Community 55|Community 55]]
- [[_COMMUNITY_Community 56|Community 56]]
- [[_COMMUNITY_Community 57|Community 57]]
- [[_COMMUNITY_Community 58|Community 58]]
- [[_COMMUNITY_Community 59|Community 59]]
- [[_COMMUNITY_Community 61|Community 61]]
- [[_COMMUNITY_Community 62|Community 62]]
- [[_COMMUNITY_Community 63|Community 63]]
- [[_COMMUNITY_Community 64|Community 64]]
- [[_COMMUNITY_Community 66|Community 66]]
- [[_COMMUNITY_Community 69|Community 69]]
- [[_COMMUNITY_Community 71|Community 71]]
- [[_COMMUNITY_Community 73|Community 73]]
- [[_COMMUNITY_Community 74|Community 74]]
- [[_COMMUNITY_Community 75|Community 75]]
- [[_COMMUNITY_Community 77|Community 77]]
- [[_COMMUNITY_Community 82|Community 82]]
- [[_COMMUNITY_Community 83|Community 83]]
- [[_COMMUNITY_Community 84|Community 84]]
- [[_COMMUNITY_Community 86|Community 86]]
- [[_COMMUNITY_Community 87|Community 87]]
- [[_COMMUNITY_Community 88|Community 88]]
- [[_COMMUNITY_Community 92|Community 92]]
- [[_COMMUNITY_Community 93|Community 93]]
- [[_COMMUNITY_Community 94|Community 94]]
- [[_COMMUNITY_Community 97|Community 97]]
- [[_COMMUNITY_Community 100|Community 100]]
- [[_COMMUNITY_Community 101|Community 101]]
- [[_COMMUNITY_Community 107|Community 107]]
- [[_COMMUNITY_Community 108|Community 108]]
- [[_COMMUNITY_Community 114|Community 114]]
- [[_COMMUNITY_Community 121|Community 121]]
- [[_COMMUNITY_Community 127|Community 127]]
- [[_COMMUNITY_Community 177|Community 177]]

## God Nodes (most connected - your core abstractions)
1. `Mathf` - 16 edges
2. `FindVoice()` - 16 edges
3. `FindVoice()` - 15 edges
4. `ApplyVoiceSettings()` - 13 edges
5. `Transform` - 12 edges
6. `GameObject` - 11 edges
7. `Input` - 11 edges
8. `RegisterBuiltinCommands()` - 11 edges
9. `RegisterResource()` - 11 edges
10. `Graphic()` - 11 edges

## Surprising Connections (you probably didn't know these)
- `Update()` --calls--> `ReverseDirection()`  [INFERRED]
  projects\PacManGame\src\game\GameController.cpp → projects\PacManGame\src\core\GameConstants.h
- `Deserialize()` --calls--> `SetLoaded()`  [INFERRED]
  src\engine\resource\MeshAsset.cpp → src\engine\core\Asset.h
- `AddSubMesh()` --calls--> `SetLoaded()`  [INFERRED]
  src\engine\resource\MeshAsset.cpp → src\engine\core\Asset.h
- `Unload()` --calls--> `SetLoaded()`  [INFERRED]
  src\engine\resource\TextureAsset.cpp → src\engine\core\Asset.h
- `Deserialize()` --calls--> `SetLoaded()`  [INFERRED]
  src\engine\resource\TextureAsset.cpp → src\engine\core\Asset.h

## Communities (343 total, 22 thin omitted)

### Community 0 - "Core Math & Types"
Cohesion: 0.04
Nodes (17): bool, float, IntPtr, MonoBehaviour, Quaternion, Prisma(), HealthManager, PlayerController (+9 more)

### Community 1 - "Platform Abstraction"
Cohesion: 0.04
Nodes (15): CreateWindow(), GetLogDirectoryPath(), GetPersistentPath(), GetTimeMicroseconds(), GetTimeSeconds(), LogToConsole(), ResetConsoleColor(), SetConsoleColor() (+7 more)

### Community 2 - "SDL3 Audio Device"
Cohesion: 0.07
Nodes (32): ApplyVoiceSettings(), AudioDeviceSDL3(), ComputeBytesPerSecond(), ComputeVoiceAttenuation(), GetAvailableDevices(), GetDeviceInfo(), GetPlaybackPosition(), Initialize() (+24 more)

### Community 3 - "PacMan Game Logic"
Cohesion: 0.07
Nodes (30): DirectionToVector(), ReverseDirection(), CalculateTargetPosition(), ChaseMode(), ChooseNextDirection(), GetBestDirection(), GetChaseTarget(), GetCurrentSpeed() (+22 more)

### Community 4 - "Asset Serialization"
Cohesion: 0.06
Nodes (22): SetLoaded(), SetPath(), CreateDefault(), Graphic(), Load(), Material(), SetBaseColor(), SetMetallic() (+14 more)

### Community 5 - "Null Audio Device"
Cohesion: 0.06
Nodes (24): AudioDeviceNull(), FindVoice(), GenerateVoiceId(), GetAvailableDevices(), GetDeviceInfo(), GetDuration(), GetPlaybackPosition(), GetVoiceState() (+16 more)

### Community 6 - "Audio Synthesis"
Cohesion: 0.06
Nodes (14): GenerateSamples(), SquareWaveGenerator(), TriangleWaveGenerator(), WaveformGenerator(), ACESToneMapping(), CalculatePBR(), DecodeNormal(), PSMain() (+6 more)

### Community 7 - "Audio Device Abstraction"
Cohesion: 0.09
Nodes (30): AudioDevice(), Calculate3DVolume(), CreateDriver(), FindVoice(), GenerateVoiceId(), GetDuration(), GetPlaybackPosition(), GetVoiceState() (+22 more)

### Community 8 - "Graphic Resource Pipeline"
Cohesion: 0.09
Nodes (36): CheckAndReloadResources(), CheckFileModifications(), CreateBuffer(), CreateDynamicBuffer(), CreatePipeline(), CreatePipelineState(), CreateSampler(), CreateShader() (+28 more)

### Community 9 - "Vulkan Resource Factory"
Cohesion: 0.07
Nodes (17): AllocateFromTexturePool(), CleanupResourcePools(), CreateBufferImpl(), CreateBuffersBatch(), CreateDynamicBuffer(), CreateSwapChainImpl(), CreateTextureFromFile(), CreateTextureFromMemory() (+9 more)

### Community 10 - "Mono Scripting Runtime"
Cohesion: 0.09
Nodes (18): BoolToMono(), ClearException(), CreateArray(), CreateBool(), CreateFloat(), CreateInstance(), CreateInt(), CreateMethodResult() (+10 more)

### Community 11 - "Render Commands"
Cohesion: 0.07
Nodes (6): SetConstantBuffer(), SetConstantData(), SetIndexBuffer(), SetIndexData(), SetVertexBuffer(), SetVertexData()

### Community 12 - "Vulkan Pipeline State"
Cohesion: 0.08
Nodes (8): Clone(), Create(), GetCacheKey(), HasShader(), Recreate(), SaveToCache(), Validate(), Vulkan()

### Community 13 - "Logging System"
Cohesion: 0.1
Nodes (15): ColorCode(), EnqueueEntry(), Flush(), FormatEntry(), GetCurrentLogScope(), GetLevelColor(), GetLevelString(), GetTimestamp() (+7 more)

### Community 14 - "Input System"
Cohesion: 0.09
Nodes (10): CreateDriver(), Initialize(), InputDevice(), IsActionJustPressed(), IsActionPressed(), IsAnyKeyDown(), IsGamepadButtonDown(), IsKeyDown() (+2 more)

### Community 15 - "PacMan Game Controller"
Cohesion: 0.12
Nodes (20): ActivatePowerMode(), AddScore(), CheckGhostCollision(), CheckLevelComplete(), CheckPelletCollision(), Initialize(), InitializeGhosts(), LoseLife() (+12 more)

### Community 16 - "UI Button Component"
Cohesion: 0.1
Nodes (10): Initialize(), OnHoverEnter(), OnHoverLeave(), OnPressed(), OnReleased(), SetHoverColor(), SetNormalColor(), SetPressedColor() (+2 more)

### Community 17 - "Vulkan Shader Compilation"
Cohesion: 0.1
Nodes (10): CreateShaderModule(), DebugSaveToFile(), DestroyShaderModule(), Disassemble(), IsFileModified(), NeedsReload(), Recompile(), RecompileFromSource() (+2 more)

### Community 18 - "Shader Build Tools"
Cohesion: 0.11
Nodes (18): operator(), Graphic(), check_dxc_compiler(), compile_shader_dxcompiler(), find_shader_files(), get_compile_command(), get_shader_profile(), load_config() (+10 more)

### Community 19 - "Sprite Renderer"
Cohesion: 0.13
Nodes (17): Render(), GetForward(), GetPosition(), GetProjectionMatrix(), GetRight(), GetUp(), GetViewMatrix(), GetViewProjectionMatrix() (+9 more)

### Community 20 - "PacMan Game Board"
Cohesion: 0.19
Nodes (21): CommandBuild(), CommandClean(), CommandExport(), CommandImport(), CommandLineEditor(), CommandPackage(), CommandRun(), CommandShowInfo() (+13 more)

### Community 21 - "Android Native Bridge"
Cohesion: 0.09
Nodes (7): CreateSwapChain(), Initialize(), RenderDeviceVulkan(), Resize(), Shutdown(), SubmitCommandBuffer(), SubmitCommandBuffers()

### Community 22 - "ECS Component System"
Cohesion: 0.17
Nodes (23): Base64Decode(), Base64Encode(), CompressGzip(), CompressZlib(), CompressZstd(), Decode(), DecompressGzip(), DecompressZlib() (+15 more)

### Community 23 - "Audio Driver Abstraction"
Cohesion: 0.13
Nodes (17): BuildChunkGeometry(), BuildGeometry(), BuildLayerGeometry(), CreateMaterial(), GetTilePosition(), GetTilesetTextureIndex(), GetTileUV(), LoadTilesetTextures() (+9 more)

### Community 24 - "Engine Core Initialization"
Cohesion: 0.11
Nodes (13): Load(), Unload(), BeginScene(), DrawQuad(), DrawString(), EndScene(), Flush(), Initialize() (+5 more)

### Community 25 - "Platform Context"
Cohesion: 0.19
Nodes (21): ParseAnimation(), ParseCollisionShapes(), ParseColor(), ParseDrawOrder(), ParseFile(), ParseLayer(), ParseLayerType(), ParseMapAttributes() (+13 more)

### Community 27 - "Block Game World"
Cohesion: 0.14
Nodes (9): HandleConsoleInput(), HasGraphicsDeviceAvailable(), OnKeyPress(), PacManApplication, PacManGame(), RenderConsoleFrame(), Run(), RunConsoleFallback() (+1 more)

### Community 28 - "Camera System"
Cohesion: 0.2
Nodes (17): CheckTunnel(), EatPellet(), EatPowerPellet(), GameBoard(), GetTile(), GridToPixel(), IsPellet(), IsPowerPellet() (+9 more)

### Community 29 - "Entity Transform System"
Cohesion: 0.14
Nodes (5): AddChunk(), RebuildMesh(), RemoveChunk(), Update(), Input

### Community 30 - "Runtime Entry Point"
Cohesion: 0.13
Nodes (7): DestroyVulkanResources(), GBuffer(), GBuffer::DepthStencilProxy, GBuffer::RenderTargetProxy, Initialize(), InitializeVulkanResources(), Resize()

### Community 31 - "Community 31"
Cohesion: 0.13
Nodes (4): GetBytesPerPixel(), GetSubresourceSize(), Vulkan(), VulkanTexture()

### Community 32 - "Community 32"
Cohesion: 0.15
Nodes (9): Cleanup(), GetCurrentRenderTarget(), GetRenderTarget(), Initialize(), Resize(), Screenshot(), SwapChainRenderTarget, ToTextureFormat() (+1 more)

### Community 33 - "Community 33"
Cohesion: 0.15
Nodes (8): ClearState(), Initialize(), InputManager(), OnEvent(), SetKeyState(), SetMouseButtonState(), SetMousePosition(), Shutdown()

### Community 34 - "Community 34"
Cohesion: 0.16
Nodes (8): Graphic(), Initialize(), InitializeDevice(), InitializeRenderPipelines(), InitializeRenderResourceManager(), RenderSystem(), SetMainPipeline(), Shutdown()

### Community 35 - "Community 35"
Cohesion: 0.15
Nodes (6): InputDriverSDL3(), MapSDLKey(), ProcessEvent(), Shutdown(), Update(), UpdateGamepads()

### Community 36 - "Community 36"
Cohesion: 0.27
Nodes (13): CreateBestDevice(), CreateDevice(), CreateNullDevice(), CreateSDL3Device(), GetDeviceFromConfig(), GetDeviceFromEnvironment(), GetDeviceVersion(), GetSupportedDevices() (+5 more)

### Community 37 - "Community 37"
Cohesion: 0.2
Nodes (8): AssetManager(), GetAssetFromCache(), Prisma(), RegisterAsset(), Shutdown(), Unload(), UnloadAll(), Update()

### Community 38 - "Community 38"
Cohesion: 0.19
Nodes (6): AddPass(), Clear(), Execute(), LogicalPipeline(), SetViewport(), SortByPriority()

### Community 39 - "Community 39"
Cohesion: 0.19
Nodes (7): Editor(), GetImGuiContext(), OnImGuiInitialize(), OnImGuiRender(), OnInitialize(), OnRender(), Prisma()

### Community 40 - "Community 40"
Cohesion: 0.21
Nodes (6): CollectStats(), Execute(), SetAmbientLight(), SetLights(), Update(), UpdatePassesCameraData()

### Community 42 - "Community 42"
Cohesion: 0.18
Nodes (5): CreateThread(), Prisma(), SetThreadName(), Shutdown(), ThreadManager()

### Community 43 - "Community 43"
Cohesion: 0.23
Nodes (6): GetBuildInfo(), GetNextValue(), GetVersion(), Parse(), ParseOption(), ShowVersion()

### Community 44 - "Community 44"
Cohesion: 0.23
Nodes (7): FromJson(), Load(), Refresh(), Save(), ToJson(), FromString(), UUID()

### Community 46 - "Community 46"
Cohesion: 0.17
Nodes (4): BlockTextureManagerImpl, SkylinePacker, TextureAtlasBuilderImpl, TextureAtlasImpl

### Community 47 - "Community 47"
Cohesion: 0.24
Nodes (7): CompileScripts(), Initialize(), LoadAssembly(), ProcessScriptAwake(), ProcessScriptStart(), ProcessScriptUpdate(), Update()

### Community 50 - "Community 50"
Cohesion: 0.36
Nodes (7): CreateFramebuffer(), CreateRenderPass(), DestroyFramebuffer(), Initialize(), Resize(), Shutdown(), ViewportRenderPass()

### Community 53 - "Community 53"
Cohesion: 0.53
Nodes (7): Build-Abi(), Check-Environment(), Show-NDKSetupHelp(), Write-ColorText(), Write-Error(), Write-Info(), Write-Warn()

### Community 55 - "Community 55"
Cohesion: 0.25
Nodes (4): Execute(), LogicalPass(), RenderUIComponent(), UIPass()

### Community 56 - "Community 56"
Cohesion: 0.33
Nodes (6): Join(), OnStart(), OnStop(), Run(), Stop(), WorkerThread()

### Community 58 - "Community 58"
Cohesion: 0.29
Nodes (4): asLong(), PrismaCraft(), Vec3i(), PrismaCraft()

### Community 59 - "Community 59"
Cohesion: 0.36
Nodes (5): AddActionOption(), CommandLineParser(), FindOption(), Parse(), ShowHelp()

### Community 62 - "Community 62"
Cohesion: 0.39
Nodes (5): fmix64(), Hash(), HashFile(), HashString(), rotl64()

### Community 63 - "Community 63"
Cohesion: 0.32
Nodes (4): CopyToTempFile(), DynamicLoader(), TryLoad(), Unload()

### Community 64 - "Community 64"
Cohesion: 0.43
Nodes (5): Engine(), Initialize(), Run(), Shutdown(), Update()

### Community 69 - "Community 69"
Cohesion: 0.33
Nodes (3): ForwardPipeline(), Shutdown(), TextureRenderTargetProxy

### Community 71 - "Community 71"
Cohesion: 0.48
Nodes (5): CreateDefaultResource(), CreateFallbackResource(), CreateDefaultMaterial(), CreateDefaultMesh(), CreateDefaultShader()

### Community 77 - "Community 77"
Cohesion: 0.47
Nodes (4): Draw(), LoadSettings(), ProjectSettingsWindow(), SaveSettings()

### Community 83 - "Community 83"
Cohesion: 0.47
Nodes (3): DestroyScope(), EndScope(), LogScope()

### Community 86 - "Community 86"
Cohesion: 0.53
Nodes (4): ParseAnimation(), ParseCollisionShapes(), ParseFile(), ParseProperties()

### Community 87 - "Community 87"
Cohesion: 0.47
Nodes (3): HandleKeyboardInput(), HandleMouseInput(), Update()

### Community 88 - "Community 88"
Cohesion: 0.47
Nodes (3): HandleMouseButton(), HandleMouseMove(), ProcessInput()

### Community 93 - "Community 93"
Cohesion: 0.6
Nodes (3): getSupportedFormats(), isFormatSupported(), loadFromFile()

### Community 94 - "Community 94"
Cohesion: 0.6
Nodes (3): OrthographicCamera(), RecalculateMatrices(), SetProjection()

### Community 100 - "Community 100"
Cohesion: 0.6
Nodes (3): CubemapTextureAsset(), loadCubemap(), loadFromFiles()

## Knowledge Gaps
- **5 isolated node(s):** `PrismaEngine`, `IntPtr`, `BasicTriangleApp`, `Packaging`, `使用DXCompiler编译着色器为SPIR-V`
  These have ≤1 connection - possible missing edges or undocumented components.
- **22 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `Mathf` connect `Audio Synthesis` to `Core Math & Types`?**
  _High betweenness centrality (0.003) - this node is a cross-community bridge._
- **What connects `PrismaEngine`, `IntPtr`, `BasicTriangleApp` to the rest of the system?**
  _5 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `Core Math & Types` be split into smaller, more focused modules?**
  _Cohesion score 0.04 - nodes in this community are weakly interconnected._
- **Should `Platform Abstraction` be split into smaller, more focused modules?**
  _Cohesion score 0.04 - nodes in this community are weakly interconnected._
- **Should `SDL3 Audio Device` be split into smaller, more focused modules?**
  _Cohesion score 0.07 - nodes in this community are weakly interconnected._
- **Should `PacMan Game Logic` be split into smaller, more focused modules?**
  _Cohesion score 0.07 - nodes in this community are weakly interconnected._
- **Should `Asset Serialization` be split into smaller, more focused modules?**
  _Cohesion score 0.06 - nodes in this community are weakly interconnected._