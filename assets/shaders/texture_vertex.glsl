uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

attribute vec3 position;
attribute vec2 texCoord;
attribute float lightLevel;

varying vec2 vTexCoord;
varying float vLightLevel;

void main() {
	gl_Position = projection * view * model * vec4(position, 1.0);
	vLightLevel = lightLevel;
	vTexCoord = texCoord;
}
