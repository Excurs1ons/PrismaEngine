using System;
using Prisma;

namespace GameScripts;

/// <summary>
/// Pure C# node-pool particle system.
/// Uses colored rectangle nodes via Interop.RenderData.
/// Max pool size: 50 particles.
/// </summary>
public static class ParticleSystem
{
    const int PoolSize = 50;
    const float ParticleSize = 3f;

    struct Particle
    {
        public Node node;
        public float life;
        public float maxLife;
        public float vx, vy;
        public float startR, startG, startB, startA;
    }

    static Particle[] s_pool = new Particle[PoolSize];
    static bool s_initialized = false;
    static Random s_rng = new Random();

    static void EnsurePool()
    {
        if (s_initialized) return;
        s_initialized = true;

        for (int i = 0; i < PoolSize; i++)
        {
            CreatePoolNode(i);
        }
    }

    static void CreatePoolNode(int index)
    {
        var n = Node.Create("__Particle__" + index);
        unsafe
        {
            uint idx = n.Handle & 0xFFFF;
            Interop.RenderData->Active[idx] = 0;
            Interop.RenderData->SizeW[idx] = ParticleSize;
            Interop.RenderData->SizeH[idx] = ParticleSize;
        }
        s_pool[index].node = n;
        s_pool[index].life = -1f;
    }

    /// <summary>
    /// Burst: spawn multiple particles at position with random velocities.
    /// </summary>
    public static void Burst(float x, float y, float r, float g, float b, int count = 8, float speed = 80f, float life = 0.5f)
    {
        EnsurePool();

        for (int i = 0; i < PoolSize && count > 0; i++)
        {
            if (s_pool[i].life > 0f) continue;

            var p = s_pool[i];
            p.node.X = x;
            p.node.Y = y;

            double angle = s_rng.NextDouble() * Math.PI * 2;
            float spd = (float)(speed * (0.5 + s_rng.NextDouble() * 0.5));
            p.vx = (float)Math.Cos(angle) * spd;
            p.vy = (float)Math.Sin(angle) * spd;

            p.life = life;
            p.maxLife = life;
            p.startR = r; p.startG = g; p.startB = b; p.startA = 1f;

            unsafe
            {
                uint idx = p.node.Handle & 0xFFFF;
                Interop.RenderData->Active[idx] = 1;
                Interop.RenderData->ColorR[idx] = r;
                Interop.RenderData->ColorG[idx] = g;
                Interop.RenderData->ColorB[idx] = b;
                Interop.RenderData->ColorA[idx] = 1f;
            }

            s_pool[i] = p;
            count--;
        }
    }

    /// <summary>
    /// Trail: spawn a single trailing particle behind a moving entity.
    /// </summary>
    public static void Trail(float x, float y, float dx, float dy, float r, float g, float b)
    {
        Burst(x, y, r, g, b, 1, 30f, 0.3f);
    }

    /// <summary>
    /// Must be called every frame from a Script's OnUpdate to advance particles.
    /// </summary>
    public static void Update(float dt)
    {
        if (!s_initialized) return;

        for (int i = 0; i < PoolSize; i++)
        {
            if (s_pool[i].life <= 0f) continue;

            var p = s_pool[i];
            p.life -= dt;
            p.node.X += p.vx * dt;
            p.node.Y += p.vy * dt;

            float t = p.life / p.maxLife;
            if (t < 0f) t = 0f;

            unsafe
            {
                uint idx = p.node.Handle & 0xFFFF;
                Interop.RenderData->ColorA[idx] = t;

                if (p.life <= 0f)
                {
                    Interop.RenderData->Active[idx] = 0;
                }
            }

            s_pool[i] = p;
        }
    }
}
