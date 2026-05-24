#version 450
// Fullscreen triangle - no vertex buffer needed, covers entire viewport
// Outputs UV coordinates for sampling
layout(location = 0) out vec2 outUV;

void main() {
    // Fullscreen triangle using vertex index
    // Generates a triangle covering the entire NDC space
    outUV = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4(outUV * 2.0 - 1.0, 0.0, 1.0);
}
