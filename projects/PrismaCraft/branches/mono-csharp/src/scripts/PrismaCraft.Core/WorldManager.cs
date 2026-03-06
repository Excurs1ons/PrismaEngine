using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Threading;
using System.Threading.Tasks;

namespace PrismaCraft.Core
{
    /// <summary>
    /// World manager - handles chunk loading, generation, and world operations
    /// Corresponds to: net.minecraft.server.level.ServerLevel
    /// </summary>
    public class WorldManager
    {
        // World properties
        private string worldName;
        private long seed;
        private int worldHeight = 256;
        private int seaLevel = 64;

        // Chunk storage
        private ConcurrentDictionary<ChunkPos, LevelChunk> chunks = new();
        private readonly object chunkLock = new object();

        // Entity management
        private Dictionary<int, Entity> entities = new();
        private int nextEntityId = 1;

        // World generator
        private WorldGenerator generator;

        // Time
        private long dayTime = 0;
        private const float DAY_LENGTH_TICKS = 24000f;

        // Spawn point
        private BlockPos spawnPoint = new BlockPos(0, 64, 0);

        /// <summary>
        /// Block position structure
        /// Corresponds to: net.minecraft.core.BlockPos
        /// </summary>
        public struct BlockPos
        {
            public int X, Y, Z;

            public BlockPos(int x, int y, int z)
            {
                X = x;
                Y = y;
                Z = z;
            }

            public override bool Equals(object obj)
            {
                return obj is BlockPos pos && X == pos.X && Y == pos.Y && Z == pos.Z;
            }

            public override int GetHashCode()
            {
                return HashCode.Combine(X, Y, Z);
            }

            public override string ToString()
            {
                return $"({X}, {Y}, {Z})";
            }
        }

        /// <summary>
        /// Chunk position structure
        /// Corresponds to: net.minecraft.core.ChunkPos
        /// </summary>
        public struct ChunkPos
        {
            public int X, Z;

            public ChunkPos(int x, int z)
            {
                X = x;
                Z = z;
            }

            public override bool Equals(object obj)
            {
                return obj is ChunkPos pos && X == pos.X && Z == pos.Z;
            }

            public override int GetHashCode()
            {
                return HashCode.Combine(X, Z);
            }

            public override string ToString()
            {
                return $"[{X}, {Z}]";
            }
        }

        /// <summary>
        /// Chunk data container
        /// Corresponds to: net.minecraft.world.level.chunk.LevelChunk
        /// </summary>
        public class LevelChunk
        {
            public ChunkPos Pos;
            public byte[] Blocks; // 16x256x16 = 65536 blocks
            public byte[] Metadata;
            public byte[] SkyLight;
            public byte[] BlockLight;
            public bool IsLoaded;
            public bool IsDirty;

            public LevelChunk(ChunkPos pos)
            {
                Pos = pos;
                Blocks = new byte[16 * 256 * 16];
                Metadata = new byte[16 * 256 * 16 / 2]; // 4 bits per block
                SkyLight = new byte[16 * 256 * 16 / 2];
                BlockLight = new byte[16 * 256 * 16 / 2];
                IsLoaded = false;
                IsDirty = true;
            }

            public int GetIndex(int x, int y, int z)
            {
                return y << 8 | z << 4 | x; // y * 256 + z * 16 + x
            }

            public byte GetBlock(int x, int y, int z)
            {
                return Blocks[GetIndex(x, y, z)];
            }

            public void SetBlock(int x, int y, int z, byte block)
            {
                Blocks[GetIndex(x, y, z)] = block;
                IsDirty = true;
            }
        }

        /// <summary>
        /// Entity base class
        /// Corresponds to: net.minecraft.world.entity.Entity
        /// </summary>
        public class Entity
        {
            public int Id;
            public string Uuid;
            public string Name;

            public float PosX, PosY, PosZ;
            public float VelX, VelY, VelZ;
            public float Yaw, Pitch;

            public bool IsOnGround;
            public bool IsRemoved;

            public WorldManager World;

            public virtual void Tick()
            {
                // Base tick logic
            }
        }

        /// <summary>
        /// Input system
        /// </summary>
        public static class Input
        {
            private static bool[] keyStates = new bool[256];
            private static bool[] keyPrevStates = new bool[256];
            private static float mouseDeltaX, mouseDeltaY;
            private static int mouseX, mouseY;
            private static bool mouseLocked = false;

            public static bool IsKeyDown(int key) => keyStates[key];
            public static bool IsKeyPressed(int key) => keyStates[key] && !keyPrevStates[key];
            public static bool IsKeyReleased(int key) => !keyStates[key] && keyPrevStates[key];
            public static float MouseDeltaX => mouseDeltaX;
            public static float MouseDeltaY => mouseDeltaY;
            public static int MouseX => mouseX;
            public static int MouseY => mouseY;
            public static bool MouseLocked => mouseLocked;

            public static void SetKeyState(int key, bool state) => keyStates[key] = state;
            public static void UpdateKeyStates()
            {
                Array.Copy(keyStates, keyPrevStates, 256);
                mouseDeltaX = 0;
                mouseDeltaY = 0;
            }

            public static void SetMousePosition(int x, int y)
            {
                if (mouseLocked)
                {
                    mouseDeltaX += x - mouseX;
                    mouseDeltaY += y - mouseY;
                }
                mouseX = x;
                mouseY = y;
            }

            public static void SetMouseLocked(bool locked) => mouseLocked = locked;
        }

        /// <summary>
        /// Time system
        /// </summary>
        public static class Time
        {
            private static float deltaTime = 0;
            private static long currentTime = 0;
            private static long lastTime = 0;

            public static float DeltaTime => deltaTime;
            public static long CurrentTime => currentTime;

            public static void Update()
            {
                lastTime = currentTime;
                currentTime = DateTimeOffset.UtcNow.ToUnixTimeMilliseconds();
                if (lastTime > 0)
                {
                    deltaTime = (currentTime - lastTime) / 1000f;
                }
            }
        }

        /// <summary>
        /// Debug/Logging system
        /// </summary>
        public static class Debug
        {
            public enum LogLevel
            {
                Info,
                Warning,
                Error,
                Debug
            }

            public static void Log(string message, LogLevel level = LogLevel.Info)
            {
                string prefix = level switch
                {
                    LogLevel.Warning => "[WARNING] ",
                    LogLevel.Error => "[ERROR] ",
                    LogLevel.Debug => "[DEBUG] ",
                    _ => "[INFO] "
                };
                Console.WriteLine(prefix + message);
            }

            public static void LogInfo(string message) => Log(message, LogLevel.Info);
            public static void LogWarning(string message) => Log(message, LogLevel.Warning);
            public static void LogError(string message) => Log(message, LogLevel.Error);
            public static void LogDebug(string message) => Log(message, LogLevel.Debug);
        }

        /// <summary>
        /// Multi-threaded chunk generator
        /// </summary>
        public class WorldGenerator
        {
            private WorldManager world;
            private int threadCount = 4;
            private Thread[] threads;
            private ConcurrentQueue<ChunkPos> generationQueue = new();
            private CancellationTokenSource cts = new();

            public WorldGenerator(WorldManager world)
            {
                this.world = world;
            }

            public void Start()
            {
                threads = new Thread[threadCount];
                for (int i = 0; i < threadCount; i++)
                {
                    threads[i] = new Thread(GenerationLoop);
                    threads[i].IsBackground = true;
                    threads[i].Start();
                }
            }

            public void Stop()
            {
                cts.Cancel();
                foreach (var thread in threads)
                {
                    thread.Join();
                }
            }

            public void RequestChunkGeneration(ChunkPos pos)
            {
                generationQueue.Enqueue(pos);
            }

            private void GenerationLoop()
            {
                while (!cts.Token.IsCancellationRequested)
                {
                    if (generationQueue.TryDequeue(out ChunkPos pos))
                    {
                        GenerateChunk(pos);
                    }
                    else
                    {
                        Thread.Sleep(10);
                    }
                }
            }

            private void GenerateChunk(ChunkPos pos)
            {
                var chunk = new LevelChunk(pos);

                // Simple terrain generation (flat with some noise)
                for (int x = 0; x < 16; x++)
                {
                    for (int z = 0; z < 16; z++)
                    {
                        int worldX = pos.X * 16 + x;
                        int worldZ = pos.Z * 16 + z;

                        // Simple height calculation
                        int height = 64;
                        for (int y = 0; y < height; y++)
                        {
                            byte block = (byte)(y < height - 4 ? 1 : 3); // Stone/Dirt
                            if (y == height - 1) block = 2; // Grass
                            chunk.SetBlock(x, y, z, block);
                        }
                    }
                }

                chunk.IsLoaded = true;
                world.chunks[pos] = chunk;
                Debug.LogDebug($"Generated chunk at {pos}");
            }
        }

        /// <summary>
        /// Constructor
        /// </summary>
        public WorldManager(string name, long seed)
        {
            worldName = name;
            this.seed = seed;
            generator = new WorldGenerator(this);
            generator.Start();

            Debug.LogInfo($"World '{name}' created with seed {seed}");
        }

        /// <summary>
        /// Get or load a chunk
        /// </summary>
        public LevelChunk GetChunk(int x, int z)
        {
            ChunkPos pos = new ChunkPos(x, z);
            if (!chunks.TryGetValue(pos, out LevelChunk chunk))
            {
                chunk = new LevelChunk(pos);
                chunks[pos] = chunk;
                generator.RequestChunkGeneration(pos);
            }
            return chunk;
        }

        /// <summary>
        /// Get block at position
        /// </summary>
        public byte GetBlock(BlockPos pos)
        {
            int chunkX = pos.X >> 4;
            int chunkZ = pos.Z >> 4;
            var chunk = GetChunk(chunkX, chunkZ);
            return chunk.GetBlock(pos.X & 15, pos.Y, pos.Z & 15);
        }

        /// <summary>
        /// Set block at position
        /// </summary>
        public void SetBlock(BlockPos pos, byte block)
        {
            int chunkX = pos.X >> 4;
            int chunkZ = pos.Z >> 4;
            var chunk = GetChunk(chunkX, chunkZ);
            chunk.SetBlock(pos.X & 15, pos.Y, pos.Z & 15, block);
        }

        /// <summary>
        /// Update world
        /// </summary>
        public void Update()
        {
            dayTime++;
            Time.Update();

            // Update all entities
            foreach (var entity in entities.Values)
            {
                if (!entity.IsRemoved)
                {
                    entity.Tick();
                }
            }

            // Remove removed entities
            entities = new Dictionary<int, Entity>(entities);
            foreach (var kvp in entities)
            {
                if (kvp.Value.IsRemoved)
                {
                    entities.Remove(kvp.Key);
                }
            }
        }

        /// <summary>
        /// Get spawn point
        /// </summary>
        public BlockPos GetSpawnPoint()
        {
            return spawnPoint;
        }

        /// <summary>
        /// Get day time (0-24000)
        /// </summary>
        public long GetDayTime()
        {
            return dayTime % 24000;
        }

        /// <summary>
        /// Shutdown
        /// </summary>
        public void Shutdown()
        {
            generator.Stop();
            Debug.LogInfo("WorldManager shut down");
        }
    }
}
