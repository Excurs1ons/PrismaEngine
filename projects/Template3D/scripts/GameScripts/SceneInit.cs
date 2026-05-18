using System;
using Prisma;

namespace GameScripts;

[Serializable]
public partial class SceneInit : Script
{
    public override void OnCreate()
    {
        var camAnchor = Node.Create("__CameraAnchor__");
        camAnchor.AddScript<CameraController>();

        Debug.Log("Template3D: C# script environment initialized.");
    }

    public override void OnUpdate(TimeContext time, InputContext input)
    {
    }
}
