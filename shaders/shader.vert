#version 450
/*
layout(location = 0) out vec3 fragColor;

vec2 positions[3] = vec2[](
	vec2(0.0, -0.5),
	vec2(0.5, 0.5),
	vec2(-0.5, 0.5)
);

vec3 colors[5] = vec3[](
	vec3(1.0, 0.0, 0.0),
	vec3(0.0, 1.0, 0.0),
	vec3(0.0, 0.0, 1.0),
	vec3(0.0, 0.0, 1.0),
	vec3(1.0, 0.0, 0.0)
);

void main() {
	gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
	fragColor = colors[gl_VertexIndex];
}
*/

layout(location = 0) in vec3 inPos;

const float PI = 3.1415926535;
float theta_x = PI / 4;
float theta_y = -PI / 6;

mat4 rotate_x = mat4(
	vec4(1.0, 0.0, 0.0, 0.0),
	vec4(0.0, cos(theta_x), -sin(theta_x), 0.0),
	vec4(0.0, sin(theta_x), cos(theta_x), 0.0),
	vec4(0.0, 0.0, 0.0, 1.0)
);

mat4 rotate_y = mat4(
	vec4(cos(theta_y), 0.0, sin(theta_y), 0.0),
	vec4(0.0, 1.0, 0.0, 0.0),
	vec4(-sin(theta_y), 0.0, cos(theta_y), 0.0),
	vec4(0.0, 0.0, 0.0, 1.0)
);

mat4 translate = mat4(
	vec4(1.0, 0.0, 0.0, 0.0),
	vec4(0.0, 1.0, 0.0, 0.0),
	vec4(0.0, 0.0, 1.0, 0.5),
	vec4(0.0, 0.0, 0.0, 1.0)
);

mat4 proj = mat4(
	vec4(1.0, 0.0, 0.0, 0.0),
	vec4(0.0, -2.0, 0.0, 0.0),
	vec4(0.0, 0.0, -2.0, -1.0),
	vec4(0.0, 0.0, 0.0, 1.0)
);

layout(binding = 0) uniform UniformBufferObject {
	mat4 model;
	mat4 view;
	mat4 proj;
} ubo;

layout(location = 1) in vec4 inColor;
layout(location = 0) out vec4 outColor;

void main() {
	// gl_Position = vec4(inPos, 1.0) * rotate_x * rotate_y * translate;
	gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPos, 1.0);
	outColor = inColor;
}
