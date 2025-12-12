using PrismaEngine;

public class HealthManager : MonoBehaviour
{
    public float maxHealth = 100.0f;
    public float currentHealth;
    public bool invulnerable = false;
    public float invulnerabilityTime = 2.0f;
    public float invulnerabilityTimer = 0.0f;

    void Start()
    {
        currentHealth = maxHealth;
        Debug.Log("HealthManager: Health initialized to " + currentHealth);
    }

    void Update()
    {
        // 更新无敌时间
        if (invulnerable && invulnerabilityTimer > 0)
        {
            invulnerabilityTimer -= Time.deltaTime;
            if (invulnerabilityTimer <= 0)
            {
                invulnerable = false;
                Debug.Log("HealthManager: Invulnerability ended");
            }
        }

        // 测试伤害（按H键）
        if (Input.GetKeyDown(KeyCode.H))
        {
            TakeDamage(10);
        }

        // 测试治疗（按T键）
        if (Input.GetKeyDown(KeyCode.T))
        {
            Heal(20);
        }
    }

    public void TakeDamage(float damage)
    {
        if (invulnerable)
        {
            Debug.Log("HealthManager: Cannot damage - invulnerable!");
            return;
        }

        currentHealth -= damage;
        currentHealth = Mathf.Clamp(currentHealth, 0, maxHealth);

        Debug.Log("HealthManager: Took " + damage + " damage. Current health: " + currentHealth);

        // 触发无敌
        if (damage > 0)
        {
            invulnerable = true;
            invulnerabilityTimer = invulnerabilityTime;
        }

        // 检查死亡
        if (currentHealth <= 0)
        {
            Die();
        }
    }

    public void Heal(float amount)
    {
        currentHealth += amount;
        currentHealth = Mathf.Clamp(currentHealth, 0, maxHealth);
        Debug.Log("HealthManager: Healed " + amount + ". Current health: " + currentHealth);
    }

    private void Die()
    {
        Debug.Log("HealthManager: Player died!");
        // TODO: 实现死亡逻辑（重置场景、显示游戏结束等）

        // 简单重置
        currentHealth = maxHealth;
        transform.Position = new Vector3(0, 1, 0);
    }

    public float GetHealthPercentage()
    {
        return currentHealth / maxHealth;
    }
}