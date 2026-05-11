using PrismaEngine;

namespace GameScripts;

public class CameraController : Script {
    public float MoveSpeed = 600f;

    public override void OnUpdate(float dt) {
        float x = 960, y = 540;
        unsafe { NativeAPI.API.GetCameraPos(&x, &y); }

        if (Input.GetKey(KeyCode.W) || Input.GetKey(KeyCode.Up))    y += MoveSpeed * dt;
        if (Input.GetKey(KeyCode.S) || Input.GetKey(KeyCode.Down))  y -= MoveSpeed * dt;
        if (Input.GetKey(KeyCode.A) || Input.GetKey(KeyCode.Left))  x -= MoveSpeed * dt;
        if (Input.GetKey(KeyCode.D) || Input.GetKey(KeyCode.Right)) x += MoveSpeed * dt;

        unsafe { NativeAPI.API.SetCameraPos(x, y); }
    }
}
