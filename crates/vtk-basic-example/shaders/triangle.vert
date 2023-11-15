#version 400

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

#include <common_view_projection.glsl>

vec3 colors[3] = vec3[](
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0)
);

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 extraColor;
layout (location = 0) out vec3 fragColor;

void main() {
   gl_Position = vec4(pos, 1.0);
   fragColor = /* colors[gl_VertexIndex]+ */ extraColor;
}
