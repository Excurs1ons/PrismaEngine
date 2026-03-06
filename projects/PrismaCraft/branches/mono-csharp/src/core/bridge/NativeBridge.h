#pragma once

#include <PrismaCraft/PrismaCraft.h>
#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/mono-config.h>
#include <string>
#include <memory>
#include <unordered_map>

namespace PrismaCraft {

// Forward declarations
class Level;
class Entity;
class BlockPos;

/**
 * Mono runtime manager for C# scripting
 * Handles initialization and cleanup of Mono runtime
 */
class PRISMACRAFT_API MonoRuntime {
public:
    MonoRuntime(const std::string& assemblyPath);
    ~MonoRuntime();

    // Initialize runtime
    bool init();

    // Load assembly
    bool loadAssembly(const std::string& assemblyName);

    // Get class from assembly
    MonoClass* getClass(const std::string& namespace_, const std::string& className);

    // Create object instance
    MonoObject* createInstance(MonoClass* klass);

    // Call method
    MonoObject* callMethod(MonoObject* object, const std::string& methodName, void** params = nullptr, int paramCount = 0);

    // Get field value
    MonoObject* getField(MonoObject* object, const std::string& fieldName);

    // Set field value
    void setField(MonoObject* object, const std::string& fieldName, MonoObject* value);

    // Get domain
    MonoDomain* getDomain() const { return domain; }

    // Is initialized
    bool isInitialized() const { return initialized; }

private:
    std::string assemblyPath;
    MonoDomain* domain = nullptr;
    MonoAssembly* assembly = nullptr;
    MonoImage* image = nullptr;
    bool initialized = false;

    bool initDomain();
    bool openAssembly();
};

/**
 * Native bridge for C# <-> C++ interop
 * Provides methods for C# scripts to interact with native C++ engine
 */
class PRISMACRAFT_API NativeBridge {
public:
    static NativeBridge& getInstance();

    // Initialize bridge with Mono runtime
    void init(MonoRuntime* runtime);

    // Level access
    void setLevel(Level* level);
    Level* getLevel();

    // Entity management
    int createEntity(const std::string& typeName);
    void removeEntity(int entityId);
    Entity* getEntity(int entityId);

    // Block access
    int getBlock(int x, int y, int z);
    void setBlock(int x, int y, int z, int blockId);

    // Chunk operations
    void loadChunk(int chunkX, int chunkZ);
    void unloadChunk(int chunkX, int chunkZ);
    bool isChunkLoaded(int chunkX, int chunkZ);

    // Input handling - bridge C++ input to C#
    void setKeyDown(int keyCode, bool state);
    void setMousePosition(int x, int y);
    void setMouseButton(int button, bool state);

    // Time
    float getDeltaTime();
    long getGameTime();

    // Register C# callbacks
    void registerUpdateCallback(MonoObject* delegate);
    void registerFixedUpdateCallback(MonoObject* delegate);
    void registerRenderCallback(MonoObject* delegate);

    // Call registered callbacks
    void callUpdateCallbacks(float deltaTime);
    void callFixedUpdateCallbacks(float fixedDeltaTime);
    void callRenderCallbacks(float deltaTime);

    // Cleanup
    void cleanup();

private:
    NativeBridge() = default;
    ~NativeBridge() = default;

    // Prevent copying
    NativeBridge(const NativeBridge&) = delete;
    NativeBridge& operator=(const NativeBridge&) = delete;

    MonoRuntime* monoRuntime = nullptr;
    Level* level = nullptr;

    std::unordered_map<int, Entity*> entities;
    int nextEntityId = 1;

    // Callback delegates
    std::vector<MonoObject*> updateCallbacks;
    std::vector<MonoObject*> fixedUpdateCallbacks;
    std::vector<MonoObject*> renderCallbacks;
};

/**
 * C# wrapper for native types
 */
namespace CSWrappers {

/**
 * BlockPos wrapper for C#
 */
class PRISMACRAFT_API BlockPosWrapper {
public:
    static MonoObject* create(int x, int y, int z);
    static std::tuple<int, int, int> get(MonoObject* obj);
};

/**
 * Vec3 wrapper for C#
 */
class PRISMACRAFT_API Vec3Wrapper {
public:
    static MonoObject* create(float x, float y, float z);
    static std::tuple<float, float, float> get(MonoObject* obj);
};

/**
 * Entity wrapper for C#
 */
class PRISMACRAFT_API EntityWrapper {
public:
    static MonoObject* wrap(Entity* entity);
    static Entity* unwrap(MonoObject* obj);
};

/**
 * Level wrapper for C#
 */
class PRISMACRAFT_API LevelWrapper {
public:
    static MonoObject* wrap(Level* level);
    static Level* unwrap(MonoObject* obj);
};

} // namespace CSWrappers

/**
 * Internal calls exported to C#
 * These are called directly from C# scripts
 */
class PRISMACRAFT_API MonoInternalCalls {
public:
    // Block operations
    static int Internal_GetBlock(int x, int y, int z);
    static void Internal_SetBlock(int x, int y, int z, int blockId);

    // Chunk operations
    static void Internal_LoadChunk(int chunkX, int chunkZ);
    static void Internal_UnloadChunk(int chunkX, int chunkZ);
    static bool Internal_IsChunkLoaded(int chunkX, int chunkZ);

    // Entity operations
    static int Internal_CreateEntity(MonoString* typeName);
    static void Internal_RemoveEntity(int entityId);
    static MonoObject* Internal_GetEntity(int entityId);

    // Time
    static float Internal_GetDeltaTime();
    static long Internal_GetGameTime();

    // Input
    static bool Internal_IsKeyDown(int keyCode);
    static void Internal_GetMousePosition(int* x, int* y);
    static bool Internal_IsMouseButtonDown(int button);

    // Register internal calls with Mono
    static void registerInternalCalls(MonoDomain* domain);
};

} // namespace PrismaCraft
