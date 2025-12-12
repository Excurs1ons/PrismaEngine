using System;
using System.Runtime.InteropServices;

namespace PrismaEngine
{
    // 基础组件类
    public class Component
    {
        public GameObject gameObject { get; internal set; }

        protected virtual void Awake() {}
        protected virtual void Start() {}
        protected virtual void Update(float deltaTime) {}
        protected virtual void OnDestroy() {}

        public T GetComponent<T>() where T : class
        {
            IntPtr componentPtr = GameObject_GetComponent(gameObject.GetHandle(), typeof(T));
            return Marshal.PtrToStructure<T>(componentPtr);
        }

        public T AddComponent<T>() where T : class, new()
        {
            IntPtr componentPtr = GameObject_AddComponent(gameObject.GetHandle(), typeof(T));
            return new T();
        }

        public bool HasComponent<T>() where T : class
        {
            return GameObject_HasComponent(gameObject.GetHandle(), typeof(T));
        }
    }

    // MonoBehaviour类 - 类似Unity
    public class MonoBehaviour : Component
    {
        protected virtual void Start() {}
        protected virtual void Update() {}
        protected virtual void FixedUpdate() {}
        protected virtual void LateUpdate() {}
        protected virtual void OnEnable() {}
        protected virtual void OnDisable() {}
        protected virtual void OnDestroy() {}
    }

    // Transform组件
    public class Transform : Component
    {
        private Vector3 position;
        private Quaternion rotation;
        private Vector3 scale = Vector3.one;

        public Vector3 Position {
            get {
                GetPosition(out float x, out float y, out float z);
                return new Vector3(x, y, z);
            }
            set {
                position = value;
                SetPosition(value.x, value.y, value.z);
            }
        }

        public Quaternion Rotation {
            get {
                GetRotation(out float x, out float y, out float z, out float w);
                return new Quaternion(x, y, z, w);
            }
            set {
                rotation = value;
                SetRotation(value.x, value.y, value.z, value.w);
            }
        }

        public Vector3 Scale {
            get {
                GetScale(out float x, out float y, out float z);
                return new Vector3(x, y, z);
            }
            set {
                scale = value;
                SetScale(value.x, value.y, value.z);
            }
        }

        public Vector3 Forward => Rotation * Vector3.forward;
        public Vector3 Right => Rotation * Vector3.right;
        public Vector3 Up => Rotation * Vector3.up;

        // 内部调用
        [MethodImpl(MethodImplOptions.InternalCall)]
        private extern void SetPosition(float x, float y, float z);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private extern void GetPosition(out float x, out float y, out float z);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private extern void SetRotation(float x, float y, float z, float w);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private extern void GetRotation(out float x, out float y, out float z, out float w);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private extern void SetScale(float x, float y, float z);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private extern void GetScale(out float x, out float y, out float z);
    }

    // GameObject类
    public class GameObject
    {
        private IntPtr handle;
        private Transform transform;

        internal GameObject(IntPtr ptr) {
            handle = ptr;
        }

        public static GameObject Create() {
            IntPtr ptr = GameObject_Create();
            return new GameObject(ptr);
        }

        public void Destroy() {
            GameObject_Destroy(handle);
        }

        public Transform Transform {
            get {
                if (transform == null) {
                    transform = GetComponent<Transform>();
                }
                return transform;
            }
        }

        internal IntPtr GetHandle() => handle;

        // 内部调用
        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern IntPtr GameObject_Create();

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern void GameObject_Destroy(IntPtr gameObject);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern IntPtr GameObject_AddComponent(IntPtr gameObject, Type componentType);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern IntPtr GameObject_GetComponent(IntPtr gameObject, Type componentType);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool GameObject_HasComponent(IntPtr gameObject, Type componentType);
    }

    // 向量3
    public struct Vector3
    {
        public float x, y, z;

        public Vector3(float x, float y, float z) {
            this.x = x;
            this.y = y;
            this.z = z;
        }

        public static Vector3 zero => new Vector3(0, 0, 0);
        public static Vector3 one => new Vector3(1, 1, 1);
        public static Vector3 forward => new Vector3(0, 0, 1);
        public static Vector3 right => new Vector3(1, 0, 0);
        public static Vector3 up => new Vector3(0, 1, 0);

        public static Vector3 operator +(Vector3 a, Vector3 b) => new Vector3(a.x + b.x, a.y + b.y, a.z + b.z);
        public static Vector3 operator -(Vector3 a, Vector3 b) => new Vector3(a.x - b.x, a.y - b.y, a.z - b.z);
        public static Vector3 operator *(Vector3 a, float d) => new Vector3(a.x * d, a.y * d, a.z * d);
        public static Vector3 operator *(float d, Vector3 a) => a * d;

        public float magnitude => Mathf.Sqrt(x * x + y * y + z * z);
        public Vector3 normalized => this / magnitude;

        public static float Dot(Vector3 a, Vector3 b) => a.x * b.x + a.y * b.y + a.z * b.z;
        public static Vector3 Cross(Vector3 a, Vector3 b) => new Vector3(
            a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x
        );
    }

    // 四元数
    public struct Quaternion
    {
        public float x, y, z, w;

        public Quaternion(float x, float y, float z, float w) {
            this.x = x;
            this.y = y;
            this.z = z;
            this.w = w;
        }

        public static Quaternion identity => new Quaternion(0, 0, 0, 1);

        public static Quaternion operator *(Quaternion a, Quaternion b) => new Quaternion(
            a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
            a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
            a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,
            a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z
        );

        public static Vector3 operator *(Quaternion q, Vector3 v) {
            Vector3 qvec = new Vector3(q.x, q.y, q.z);
            float crossVal = 2.0f * Vector3.Dot(qvec, v);
            return v + crossVal * qvec + q.w * (crossVal * qvec + Vector3.Cross(qvec, v));
        }
    }

    // 数学工具类
    public static class Mathf
    {
        public const float PI = 3.14159274f;
        public const float Deg2Rad = PI / 180f;
        public const float Rad2Deg = 180f / PI;
        public const float Epsilon = 1e-6f;

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern float Sin(float value);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern float Cos(float value);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern float Tan(float value);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern float Abs(float value);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern float Min(float a, float b);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern float Max(float a, float b);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern float Clamp(float value, float min, float max);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern float Lerp(float a, float b, float t);

        public static float Sqrt(float value) => (float)Math.Sqrt(value);
        public static float Pow(float x, float y) => (float)Math.Pow(x, y);

        public static float Asin(float value) => (float)Math.Asin(value);
        public static float Acos(float value) => (float)Math.Acos(value);
        public static float Atan(float value) => (float)Math.Atan(value);
        public static float Atan2(float y, float x) => (float)Math.Atan2(y, x);
    }

    // 时间类
    public static class Time
    {
        public static float deltaTime => GetDeltaTime();
        public static float time => GetTime();
        public static float timeScale = 1.0f;
        public static float fixedDeltaTime = 0.02f;

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern float GetDeltaTime();

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern float GetTime();
    }

    // 输入类
    public static class Input
    {
        public static bool GetKey(KeyCode key) => Input_GetKey((int)key);
        public static bool GetKeyDown(KeyCode key) => Input_GetKeyDown((int)key);
        public static bool GetKeyUp(KeyCode key) => Input_GetKeyUp((int)key);

        public static bool GetMouseButton(int button) => Input_GetMouseButton(button);

        public static float mouseX => Input_GetMouseX();
        public static float mouseY => Input_GetMouseY();
        public static Vector3 mousePosition => new Vector3(mouseX, mouseY, 0);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern bool Input_GetKey(int keyCode);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern bool Input_GetKeyDown(int keyCode);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern bool Input_GetKeyUp(int keyCode);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern bool Input_GetMouseButton(int button);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern float Input_GetMouseX();

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern float Input_GetMouseY();
    }

    // 按键码枚举
    public enum KeyCode
    {
        A = 65, B, C, D, E, F, G, H, I, J, K, L, M,
        N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
        Space = 32,
        Enter = 257,
        Tab = 258,
        Backspace = 259,
        Delete = 261,
        Escape = 256,
        Left = 263,
        Right = 262,
        Up = 265,
        Down = 264,
        LeftShift = 340,
        RightShift = 344,
        LeftCtrl = 341,
        RightCtrl = 345,
        LeftAlt = 342,
        RightAlt = 346
    }

    // 调试类
    public static class Debug
    {
        public static void Log(object message) => Debug_Log(message?.ToString());
        public static void LogWarning(object message) => Debug_LogWarning(message?.ToString());
        public static void LogError(object message) => Debug_LogError(message?.ToString());

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern void Debug_Log(string message);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern void Debug_LogWarning(string message);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern void Debug_LogError(string message);
    }
}