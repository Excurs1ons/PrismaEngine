#version 460
#extension GL_EXT_ray_tracing : require

// ======== Payload 声明（与 rgen 匹配） ========
struct HitPayload {
    vec3 color;
    vec3 normal;
    float emissive;
    int objIndex;
    float t;
    int hitType;
};

layout(location = 4) rayPayloadInEXT HitPayload prd;

// ======== 场景数据绑定 ========
struct SceneObject {
    vec4 p0;
    vec4 p1;
    vec4 p2;
    vec4 color;
    mat4 worldMatrix;
    mat4 invWorldMatrix;
};

layout(std430, binding = 3) readonly buffer SceneData {
    int objectCount;
    SceneObject objects[];
} scene;

struct Vertex {
    vec3 pos;
    float pad0;
    vec3 nrm;
    float pad1;
};

struct Triangle {
    Vertex vertices[3];
};

layout(std430, binding = 4) readonly buffer TriangleBuffer {
    int triangleCount;
    Triangle triangles[];
} triBuf;

void main() {
    int objIdx = int(gl_InstanceCustomIndexEXT);
    if (objIdx < 0 || objIdx >= scene.objectCount) return;

    SceneObject obj = scene.objects[objIdx];
    // type 4 = mesh — 只有网格物体走 AS 命中
    int type = int(obj.p0.w);
    if (type != 4) return;

    int firstTri = int(obj.p1.x);
    int triIdx = firstTri + int(gl_PrimitiveID);
    if (triIdx >= triBuf.triangleCount) return;

    Triangle tri = triBuf.triangles[triIdx];

    vec3 v0 = tri.vertices[0].pos;
    vec3 v1 = tri.vertices[1].pos;
    vec3 v2 = tri.vertices[2].pos;
    vec3 n0 = tri.vertices[0].nrm;
    vec3 n1 = tri.vertices[1].nrm;
    vec3 n2 = tri.vertices[2].nrm;

    // ======== 计算重心坐标 ========
    vec3 worldHit = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
    vec3 objHit = vec3(gl_WorldToObjectEXT * vec4(worldHit, 1.0));

    vec3 ov0v1 = v1 - v0;
    vec3 ov0v2 = v2 - v0;
    float d00 = dot(ov0v1, ov0v1);
    float d01 = dot(ov0v1, ov0v2);
    float d11 = dot(ov0v2, ov0v2);
    float d20 = dot(objHit - v0, ov0v1);
    float d21 = dot(objHit - v0, ov0v2);
    float denom = d00 * d11 - d01 * d01;
    if (abs(denom) < 1e-10) return;
    float v = (d11 * d20 - d01 * d21) / denom;
    float w = (d00 * d21 - d01 * d20) / denom;
    float u = 1.0 - v - w;

    // ======== 法线插值 ========
    vec3 localNormal = u * n0 + v * n1 + w * n2;
    if (dot(localNormal, localNormal) < 0.1) {
        localNormal = normalize(cross(v1 - v0, v2 - v0));
    }
    // 变换到世界空间（逆转置矩阵）
    mat3 worldToObjT = transpose(mat3(gl_WorldToObjectEXT));
    vec3 worldNormal = normalize(worldToObjT * localNormal);

    // 确保法线朝向射线来源方向
    vec3 rayDir = gl_WorldRayDirectionEXT;
    if (dot(rayDir, worldNormal) > 0.0) worldNormal = -worldNormal;

    // ======== 填充 Payload ========
    prd.color = obj.color.rgb;
    prd.normal = worldNormal;
    prd.emissive = obj.color.w;
    prd.objIndex = objIdx;
    prd.t = gl_HitTEXT;
    prd.hitType = 1;
}
