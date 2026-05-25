using System.Collections.Concurrent;

namespace NeoEditor.Core;

public sealed class EngineToUIQueue
{
    private readonly ConcurrentQueue<Action> _queue = new();

    public void Enqueue(Action item) => _queue.Enqueue(item);

    public bool TryDequeue(out Action item)
    {
        if (_queue.TryDequeue(out var dq))
        {
            item = dq!;
            return true;
        }
        item = null!;
        return false;
    }

    public int Drain(Action<Action> handler)
    {
        int count = 0;
        while (_queue.TryDequeue(out var action))
        {
            handler(action!);
            count++;
        }
        return count;
    }
}

public sealed class UIToEngineQueue
{
    private readonly ConcurrentQueue<Action> _queue = new();

    public void Enqueue(Action item) => _queue.Enqueue(item);

    public bool TryDequeue(out Action item)
    {
        if (_queue.TryDequeue(out var dq))
        {
            item = dq!;
            return true;
        }
        item = null!;
        return false;
    }

    public int Drain(Action<Action> handler)
    {
        int count = 0;
        while (_queue.TryDequeue(out var action))
        {
            handler(action!);
            count++;
        }
        return count;
    }
}
