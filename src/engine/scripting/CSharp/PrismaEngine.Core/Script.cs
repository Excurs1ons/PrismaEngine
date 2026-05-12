using System;

namespace PrismaEngine;

public enum UpdatePhase { None = 0, Update = 1, LateUpdate = 2 }

public abstract class Script
{
    public Node node { get; internal set; }

    internal Script? _nextScript;
    internal uint _entityIndex;

    public abstract uint TypeId { get; }

    // Cherno Optimization: 双缓冲变换访问。
    // Getter 读取上一帧锁定的数据（ReadBuffer），Setter 写入这一帧的新数据（WriteBuffer）。
    // 这保证了在并行 Update 期间，所有脚本看到的数据视图都是一致的。
    public unsafe Vector2 Position 
    { 
        get => new Vector2(NativeAPI.TransformBuffer_Read->PosX[_entityIndex], NativeAPI.TransformBuffer_Read->PosY[_entityIndex]); 
        set 
        { 
            NativeAPI.TransformBuffer_Write->PosX[_entityIndex] = value.X; 
            NativeAPI.TransformBuffer_Write->PosY[_entityIndex] = value.Y; 
        } 
    }

    public unsafe float Rotation 
    { 
        get => NativeAPI.TransformBuffer_Read->Rotation[_entityIndex]; 
        set => NativeAPI.TransformBuffer_Write->Rotation[_entityIndex] = value; 
    }

    public unsafe Vector2 Scale 
    { 
        get => new Vector2(NativeAPI.TransformBuffer_Read->ScaleX[_entityIndex], NativeAPI.TransformBuffer_Read->ScaleY[_entityIndex]); 
        set 
        { 
            NativeAPI.TransformBuffer_Write->ScaleX[_entityIndex] = value.X; 
            NativeAPI.TransformBuffer_Write->ScaleY[_entityIndex] = value.Y; 
        } 
    }

    internal bool _started;
    public virtual UpdatePhase ExecutionPhase => UpdatePhase.Update;

    public virtual void OnCreate() { }
    public virtual void OnStart() { }
    public virtual void OnUpdate(TimeContext time, InputContext input) { }
    public virtual void OnLateUpdate(TimeContext time, InputContext input) { }
    public virtual void OnDestroy() { }

    // 序列化支持 / Serialization Support
    public virtual void OnSerialize(System.Text.Json.Utf8JsonWriter writer) { }
    public virtual void OnDeserialize(ref System.Text.Json.Utf8JsonReader reader) { }
}
