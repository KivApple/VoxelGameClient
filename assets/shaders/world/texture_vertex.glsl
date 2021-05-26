#version 100
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform sampler2D chunkTexture;

attribute vec3 position;
attribute vec2 texCoord;

varying vec2 vTexCoord;
varying float vLightLevel;

float round(float v) {
	return floor(v + 0.5);
}

vec4 voxel(vec3 coord) {
	float x = round(coord.x) + 1.0;
	float y = round(coord.y) + 1.0;
	float z = round(coord.z) + 1.0;
	
	float x0 = mod(y, 5.0) / 5.0;
	float y0 = floor(y / 5.0) / 5.0;
	return texture2D(
		chunkTexture,
		vec2(x0 + (x + 0.5) / (5.0 * 18.0), y0 + (z + 0.5) / (5.0 * 18.0))
	);
}

void main() {
	gl_Position = projection * view * model * vec4(position, 1.0);
	vTexCoord = texCoord;
	vLightLevel = 0.0;
	for (float dz = -1.0; dz <= 1.0; dz += 1.0) {
		for (float dy = -1.0; dy <= 1.0; dy += 1.0) {
			for (float dx = -1.0; dx <= 1.0; dx += 1.0) {
				vLightLevel = max(vLightLevel, voxel(position + vec3(dx, dy, dz) * 0.1).r);
			}
		}
	}
}
