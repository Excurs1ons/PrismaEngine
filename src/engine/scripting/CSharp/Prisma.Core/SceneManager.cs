using System;
using System.Runtime.InteropServices;
using System.Text;

namespace Prisma;

public static unsafe class SceneManager
{
    public static void SwitchScene(string sceneName, float fadeMs = 500)
    {
        byte[] bytes = Encoding.UTF8.GetBytes(sceneName + "\0");
        fixed (byte* p = bytes)
        {
            Interop.API.SceneSwitchScene(p, fadeMs);
        }
    }

    public static string GetCurrentSceneName()
    {
        byte* result = Interop.API.SceneGetCurrentSceneName();
        return result != null ? Marshal.PtrToStringUTF8((IntPtr)result) : "";
    }

    public static void SetTransitionData(string key, string value)
    {
        byte[] keyBytes = Encoding.UTF8.GetBytes(key + "\0");
        byte[] valBytes = Encoding.UTF8.GetBytes(value + "\0");
        fixed (byte* pk = keyBytes)
        fixed (byte* pv = valBytes)
        {
            Interop.API.SceneSetTransitionData(pk, pv);
        }
    }

    public static string GetTransitionData(string key)
    {
        byte[] keyBytes = Encoding.UTF8.GetBytes(key + "\0");
        byte* result;
        fixed (byte* pk = keyBytes)
        {
            result = Interop.API.SceneGetTransitionData(pk);
        }
        return result != null ? Marshal.PtrToStringUTF8((IntPtr)result) : null;
    }

    public static bool HasTransitionData(string key)
    {
        byte[] keyBytes = Encoding.UTF8.GetBytes(key + "\0");
        fixed (byte* pk = keyBytes)
        {
            return Interop.API.SceneHasTransitionData(pk);
        }
    }

    public static void ClearTransitionData() => Interop.API.SceneClearTransitionData();
}
