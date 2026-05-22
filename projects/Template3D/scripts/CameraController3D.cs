using System;
using Prisma;

namespace Template3D;

[Serializable]
public partial class CameraController3D : Script
{
    public float MoveSpeed = 5.0f;
    public float MouseSensitivity = 0.1f;
    public float ScrollSensitivity = 5.0f;

    private bool _mouseCaptured;
    private bool _neeEnabled;

    // Edge-detection for one-shot pipeline control keys
    private bool _prevTab, _prevR, _prevN, _prevB, _prevLB, _prevRB;

    public override UpdatePhase ExecutionPhase => UpdatePhase.Update;

    public override void OnUpdate(TimeContext time, InputContext input)
    {
        // ========== 1. Mouse Look (3D rotation via delta) ==========
        float deltaX = Input.MouseDeltaX;
        float deltaY = Input.MouseDeltaY;
        if (deltaX != 0 || deltaY != 0)
        {
            unsafe
            {
                Interop.API.SetCameraRotation(
                    deltaY * MouseSensitivity,   // pitch (vertical)
                    deltaX * MouseSensitivity    // yaw   (horizontal)
                );
            }
        }

        // ========== 2. WASDQE Movement (local-space) ==========
        float moveAmount = MoveSpeed * time.DeltaTime;
        float forward = 0, right = 0, up = 0;

        if (input.GetKey(KeyCode.W)) forward += moveAmount;
        if (input.GetKey(KeyCode.S)) forward -= moveAmount;
        if (input.GetKey(KeyCode.A)) right -= moveAmount;
        if (input.GetKey(KeyCode.D)) right += moveAmount;
        if (input.GetKey(KeyCode.Q)) up -= moveAmount;
        if (input.GetKey(KeyCode.E)) up += moveAmount;

        if (forward != 0 || right != 0 || up != 0)
        {
            unsafe { Interop.API.MoveCameraLocal(forward, right, up); }
        }

        // ========== 3. Mouse Scroll → Movement Speed ==========
        float scroll = Input.MouseScrollY;
        if (scroll != 0)
        {
            MoveSpeed += scroll * ScrollSensitivity * time.DeltaTime;
            MoveSpeed = Mathf.Max(0.1f, MoveSpeed);
        }

        // ========== 4. Mouse Capture (Tab toggle, Esc release) ==========
        bool tabDown = input.GetKey(KeyCode.Tab);
        if (tabDown && !_prevTab)
        {
            _mouseCaptured = !_mouseCaptured;
            unsafe { Interop.API.SetMouseCapture(_mouseCaptured); }
        }
        _prevTab = tabDown;

        if (input.GetKey(KeyCode.Escape) && _mouseCaptured)
        {
            unsafe { Interop.API.SetMouseCapture(false); }
            _mouseCaptured = false;
        }

        // ========== 5. Pipeline Control Keys (edge-triggered) ==========
        bool rDown = input.GetKey(KeyCode.R);
        if (rDown && !_prevR) unsafe { Interop.API.PtResetAccumulation(); }
        _prevR = rDown;

        bool nDown = input.GetKey(KeyCode.N);
        if (nDown && !_prevN)
        {
            _neeEnabled = !_neeEnabled;
            unsafe { Interop.API.PtSetNEE(_neeEnabled); }
        }
        _prevN = nDown;

        bool bDown = input.GetKey(KeyCode.B);
        if (bDown && !_prevB) unsafe { Interop.API.PtCycleMode(); }
        _prevB = bDown;

        const KeyCode keyLB = (KeyCode)47;
        bool lbDown = input.GetKey(keyLB);
        if (lbDown && !_prevLB)
        {
            unsafe
            {
                uint s = Interop.API.PtGetMaxSamples();
                Interop.API.PtSetMaxSamples(Math.Max(1u, s / 2u));
            }
        }
        _prevLB = lbDown;

        const KeyCode keyRB = (KeyCode)48;
        bool rbDown = input.GetKey(keyRB);
        if (rbDown && !_prevRB)
        {
            unsafe
            {
                uint s = Interop.API.PtGetMaxSamples();
                Interop.API.PtSetMaxSamples(Math.Max(1u, s * 2u));
            }
        }
        _prevRB = rbDown;
    }
}
