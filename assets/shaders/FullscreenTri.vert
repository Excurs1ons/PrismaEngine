#version 450

layout(location = 0) out vec2 v_TexCoord;

void main() {
    // Full-screen triangle using vertex ID (no VBO needed)
    // Generates a triangle covering the entire NDC:
    //   Vertex 0: (-1, -1) -> TexCoord (0, 0)
    //   Vertex 1: ( 3, -1) -> TexCoord (2, 0)
    //   Vertex 2: (-1,  3) -> TexCoord (0, 2)
    // The rasterizer clips anything outside the viewport, producing
    // a perfect full-screen quad through the first two vertices.
    uint idx = gl_VertexIndex;
    float x = float(idx & 1u) * 4.0 - 1.0;
    float y = float(idx & 2u) * 2.0 - 1.0;
    v_TexCoord = vec2(x * 0.5 + 0.5, y * 0.5 + 0.5);
    gl_Position = vec4(x, y, 0.0, 1.0);
}
