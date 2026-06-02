using System;
using System.Runtime.InteropServices;
using System.Text;

namespace Prisma;

public class SpriteAnimation : IDisposable
{
    private uint m_animId;

    public SpriteAnimation(string name)
    {
        byte[] bytes = Encoding.UTF8.GetBytes(name + "\0");
        fixed (byte* p = bytes)
        {
            m_animId = Interop.API.SpriteAnimationCreate(p);
        }
    }

    public void AddFrame(float rectX, float rectY, float width, float height, float duration)
    {
        Interop.API.SpriteAnimationAddFrame(m_animId, rectX, rectY, width, height, duration);
    }

    public void SetLooping(bool looping)
    {
        Interop.API.SpriteAnimationSetLooping(m_animId, looping);
    }

    public void Dispose()
    {
        m_animId = 0;
    }
}

public class SpriteAnimationComponent
{
    private uint m_componentId;

    public SpriteAnimationComponent(uint componentId)
    {
        m_componentId = componentId;
    }

    public void Play(string animationName, bool restart = false)
    {
        byte[] bytes = Encoding.UTF8.GetBytes(animationName + "\0");
        fixed (byte* p = bytes)
        {
            Interop.API.SpriteAnimationPlay(m_componentId, p, restart);
        }
    }

    public void Stop() => Interop.API.SpriteAnimationStop(m_componentId);
    public void Pause() => Interop.API.SpriteAnimationPause(m_componentId);
    public bool IsPlaying => Interop.API.SpriteAnimationIsPlaying(m_componentId);

    public void SetSpeed(float speed)
    {
        Interop.API.SpriteAnimationSetSpeed(m_componentId, speed);
    }

    public static SpriteAnimationComponent AddToNode(uint nodeId)
    {
        uint compId = Interop.API.NodeAddSpriteAnimation(nodeId);
        return compId != 0 ? new SpriteAnimationComponent(compId) : null;
    }
}
