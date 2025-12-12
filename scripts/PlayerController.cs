using PrismaEngine;

public class PlayerController : MonoBehaviour
{
    public float moveSpeed = 5.0f;
    public float rotateSpeed = 180.0f;
    public float jumpForce = 5.0f;
    public float gravity = -9.81f;

    private Vector3 velocity;
    private bool isGrounded = false;

    void Start()
    {
        Debug.Log("PlayerController started!");
    }

    void Update()
    {
        // 获取输入
        float horizontal = Input.GetKey(KeyCode.A) ? -1.0f : (Input.GetKey(KeyCode.D) ? 1.0f : 0.0f);
        float vertical = Input.GetKey(KeyCode.S) ? -1.0f : (Input.GetKey(KeyCode.W) ? 1.0f : 0.0f);

        // 移动
        Vector3 movement = new Vector3(horizontal, 0, vertical);
        movement.Normalize();
        movement *= moveSpeed * Time.deltaTime;

        // 应用移动
        transform.Position += movement;

        // 旋转面向鼠标
        Vector3 mousePosition = new Vector3(Input.mouseX, 0, Input.mouseY);
        Vector3 direction = mousePosition - transform.Position;
        direction.y = 0;

        if (direction.magnitude > 0.1f)
        {
            direction.Normalize();
            Quaternion targetRotation = Quaternion.LookRotation(direction);
            transform.Rotation = Quaternion.Slerp(transform.Rotation, targetRotation, rotateSpeed * Time.deltaTime);
        }

        // 跳跃
        if (Input.GetKeyDown(KeyCode.Space) && isGrounded)
        {
            velocity.y = jumpForce;
            isGrounded = false;
        }

        // 应用重力
        velocity.y += gravity * Time.deltaTime;
        transform.Position.y += velocity.y * Time.deltaTime;

        // 检测地面（简单实现）
        if (transform.Position.y <= 1.0f)
        {
            transform.Position.y = 1.0f;
            velocity.y = 0;
            isGrounded = true;
        }
    }

    void OnDestroy()
    {
        Debug.Log("PlayerController destroyed!");
    }
}