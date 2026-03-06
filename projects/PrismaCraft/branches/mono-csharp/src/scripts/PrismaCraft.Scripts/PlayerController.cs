using System;
using PrismaCraft.Core;

namespace PrismaCraft.Scripts
{
    /// <summary>
    /// Player controller - handles player input, movement, and interaction
    /// Corresponds to: net.minecraft.server.level.ServerPlayer
    /// </summary>
    public class PlayerController
    {
        private WorldManager world;
        private WorldManager.Entity playerEntity;
        private WorldManager.BlockPos playerBlockPos;

        // Movement
        private float moveSpeed = 4.5f;
        private float jumpForce = 8.0f;
        private float gravity = -20.0f;
        private bool isGrounded = true;

        // Input state
        private bool forward, backward, left, right, jump, sprint;

        // Camera
        private float cameraYaw = 0;
        private float cameraPitch = 0;
        private const float MAX_PITCH = 89f;
        private const float MIN_PITCH = -89f;

        // Block interaction
        private float reachDistance = 5f;
        private WorldManager.BlockPos? targetedBlock = null;

        /// <summary>
        /// Constructor
        /// </summary>
        public PlayerController(WorldManager world)
        {
            this.world = world;
            InitializePlayer();
        }

        /// <summary>
        /// Initialize player entity
        /// </summary>
        private void InitializePlayer()
        {
            var spawn = world.GetSpawnPoint();
            playerEntity = new WorldManager.Entity
            {
                Id = 1,
                Uuid = Guid.NewGuid().ToString(),
                Name = "Player",
                PosX = spawn.X,
                PosY = spawn.Y,
                PosZ = spawn.Z,
                VelX = 0,
                VelY = 0,
                VelZ = 0,
                Yaw = 0,
                Pitch = 0,
                IsOnGround = true,
                IsRemoved = false,
                World = world
            };

            UpdatePlayerBlockPos();
            WorldManager.Debug.LogInfo("Player initialized at spawn point");
        }

        /// <summary>
        /// Update player block position from floating position
        /// </summary>
        private void UpdatePlayerBlockPos()
        {
            playerBlockPos = new WorldManager.BlockPos(
                (int)Math.Floor(playerEntity.PosX),
                (int)Math.Floor(playerEntity.PosY),
                (int)Math.Floor(playerEntity.PosZ)
            );
        }

        /// <summary>
        /// Update controller - call each frame
        /// </summary>
        public void Update()
        {
            HandleInput();
            UpdateMovement();
            UpdateCamera();
            CheckBlockInteraction();
        }

        /// <summary>
        /// Handle input
        /// </summary>
        private void HandleInput()
        {
            // Keyboard input (WASD)
            forward = WorldManager.Input.IsKeyDown(87); // W
            backward = WorldManager.Input.IsKeyDown(83); // S
            left = WorldManager.Input.IsKeyDown(65); // A
            right = WorldManager.Input.IsKeyDown(68); // D
            jump = WorldManager.Input.IsKeyDown(32); // Space
            sprint = WorldManager.Input.IsKeyDown(340); // Left Shift or Ctrl

            // Mouse input
            float sensitivity = 0.1f;
            cameraYaw -= WorldManager.Input.MouseDeltaX * sensitivity;
            cameraPitch -= WorldManager.Input.MouseDeltaY * sensitivity;
            cameraPitch = Math.Clamp(cameraPitch, MIN_PITCH, MAX_PITCH);

            // Update entity rotation
            playerEntity.Yaw = cameraYaw;
            playerEntity.Pitch = cameraPitch;
        }

        /// <summary>
        /// Update movement
        /// </summary>
        private void UpdateMovement()
        {
            float dt = WorldManager.Time.DeltaTime;
            if (dt <= 0) return;

            float speed = moveSpeed;
            if (sprint) speed *= 1.5f;

            // Calculate movement direction based on camera yaw
            float yawRad = cameraYaw * (float)Math.PI / 180f;
            float sinYaw = (float)Math.Sin(yawRad);
            float cosYaw = (float)Math.Cos(yawRad);

            // Forward/backward
            if (forward)
            {
                playerEntity.VelX += sinYaw * speed * dt;
                playerEntity.VelZ += cosYaw * speed * dt;
            }
            if (backward)
            {
                playerEntity.VelX -= sinYaw * speed * dt;
                playerEntity.VelZ -= cosYaw * speed * dt;
            }

            // Strafe left/right
            if (left)
            {
                playerEntity.VelX -= cosYaw * speed * dt;
                playerEntity.VelZ += sinYaw * speed * dt;
            }
            if (right)
            {
                playerEntity.VelX += cosYaw * speed * dt;
                playerEntity.VelZ -= sinYaw * speed * dt;
            }

            // Apply gravity
            playerEntity.VelY += gravity * dt;

            // Jump
            if (jump && isGrounded)
            {
                playerEntity.VelY = jumpForce * dt;
                isGrounded = false;
            }

            // Apply velocity with collision detection
            float newX = playerEntity.PosX + playerEntity.VelX;
            float newY = playerEntity.PosY + playerEntity.VelY;
            float newZ = playerEntity.PosZ + playerEntity.VelZ;

            // Simple collision check
            if (!CheckCollision(newX, playerEntity.PosY, playerEntity.PosZ))
            {
                playerEntity.PosX = newX;
            }
            else
            {
                playerEntity.VelX = 0;
            }

            if (!CheckCollision(playerEntity.PosX, newY, playerEntity.PosZ))
            {
                playerEntity.PosY = newY;
                isGrounded = false;
            }
            else
            {
                if (playerEntity.VelY < 0)
                {
                    isGrounded = true;
                }
                playerEntity.VelY = 0;
            }

            if (!CheckCollision(playerEntity.PosX, playerEntity.PosY, newZ))
            {
                playerEntity.PosZ = newZ;
            }
            else
            {
                playerEntity.VelZ = 0;
            }

            // Damping
            playerEntity.VelX *= 0.9f;
            playerEntity.VelZ *= 0.9f;

            UpdatePlayerBlockPos();
        }

        /// <summary>
        /// Check collision at position
        /// </summary>
        private bool CheckCollision(float x, float y, float z)
        {
            // Check player bounding box (0.6x1.8x0.6)
            float halfWidth = 0.3f;
            float height = 1.8f;

            for (int bx = (int)Math.Floor(x - halfWidth); bx <= (int)Math.Floor(x + halfWidth); bx++)
            {
                for (int bz = (int)Math.Floor(z - halfWidth); bz <= (int)Math.Floor(z + halfWidth); bz++)
                {
                    for (int by = (int)Math.Floor(y); by <= (int)Math.Floor(y + height); by++)
                    {
                        byte block = world.GetBlock(new WorldManager.BlockPos(bx, by, bz));
                        if (block != 0) // Not air
                        {
                            return true;
                        }
                    }
                }
            }
            return false;
        }

        /// <summary>
        /// Update camera
        /// </summary>
        private void UpdateCamera()
        {
            // Camera position is at player eyes (1.62 blocks above feet)
            // Camera rotation matches player yaw/pitch
            // This would sync with the rendering engine
        }

        /// <summary>
        /// Check block interaction (raycast)
        /// </summary>
        private void CheckBlockInteraction()
        {
            targetedBlock = null;

            float yawRad = cameraYaw * (float)Math.PI / 180f;
            float pitchRad = cameraPitch * (float)Math.PI / 180f;

            // Direction vector from camera rotation
            float dirX = (float)(Math.Sin(yawRad) * Math.Cos(pitchRad));
            float dirY = (float)Math.Sin(pitchRad);
            float dirZ = (float)(Math.Cos(yawRad) * Math.Cos(pitchRad));

            // Raycast
            float step = 0.1f;
            for (float d = 0; d < reachDistance; d += step)
            {
                float x = playerEntity.PosX + dirX * d;
                float y = playerEntity.PosY + 1.62f + dirY * d; // Eye level
                float z = playerEntity.PosZ + dirZ * d;

                var blockPos = new WorldManager.BlockPos(
                    (int)Math.Floor(x),
                    (int)Math.Floor(y),
                    (int)Math.Floor(z)
                );

                byte block = world.GetBlock(blockPos);
                if (block != 0)
                {
                    targetedBlock = blockPos;
                    break;
                }
            }
        }

        /// <summary>
        /// Break targeted block
        /// </summary>
        public void BreakTargetedBlock()
        {
            if (targetedBlock.HasValue)
            {
                world.SetBlock(targetedBlock.Value, 0); // Set to air
                WorldManager.Debug.LogInfo($"Broke block at {targetedBlock.Value}");
                targetedBlock = null;
            }
        }

        /// <summary>
        /// Place block at targeted position
        /// </summary>
        public void PlaceBlock(byte blockType)
        {
            if (targetedBlock.HasValue)
            {
                var pos = targetedBlock.Value;
                // Place block adjacent to targeted block
                world.SetBlock(new WorldManager.BlockPos(pos.X + 1, pos.Y, pos.Z), blockType);
                WorldManager.Debug.LogInfo($"Placed block at {pos.X + 1}, {pos.Y}, {pos.Z}");
            }
        }

        /// <summary>
        /// Get player entity
        /// </summary>
        public WorldManager.Entity GetPlayerEntity()
        {
            return playerEntity;
        }

        /// <summary>
        /// Get player position
        /// </summary>
        public (float x, float y, float z) GetPosition()
        {
            return (playerEntity.PosX, playerEntity.PosY, playerEntity.PosZ);
        }

        /// <summary>
        /// Get camera rotation
        /// </summary>
        public (float yaw, float pitch) GetCameraRotation()
        {
            return (cameraYaw, cameraPitch);
        }

        /// <summary>
        /// Is player on ground
        /// </summary>
        public bool IsGrounded()
        {
            return isGrounded;
        }
    }
}
