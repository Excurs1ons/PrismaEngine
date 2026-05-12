using System;
using System.Collections.Generic;

namespace PrismaEngine;

public static class ScriptRegistry
{
    private static readonly Dictionary<uint, Func<Script>> _factories = new();
    private static readonly Dictionary<string, uint> _nameToId = new();
    private static readonly Dictionary<Type, uint> _typeToId = new();

    public static void Register<T>(uint typeId) where T : Script, new()
    {
        _factories[typeId] = () => new T();
        _nameToId[typeof(T).Name] = typeId;
        _typeToId[typeof(T)] = typeId;
    }

    public static Script? Create(uint typeId)
    {
        if (_factories.TryGetValue(typeId, out var factory)) return factory();
        return null;
    }

    public static uint GetIdByName(string name)
    {
        if (_nameToId.TryGetValue(name, out var id)) return id;
        return 0;
    }

    public static uint GetId<T>() => _typeToId.TryGetValue(typeof(T), out var id) ? id : 0;
    public static uint GetId(Type type) => _typeToId.TryGetValue(type, out var id) ? id : 0;
}
