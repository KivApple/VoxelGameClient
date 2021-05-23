#version 100
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

attribute vec3 position;
attribute vec2 texCoord;
attribute vec4 color;

varying vec2 vTexCoord;
varying vec4 vColor;

void main() {
	gl_Position = projection * view * model * vec4(position, 1.0);
	vTexCoord = texCoord;
	vColor = color;
}
