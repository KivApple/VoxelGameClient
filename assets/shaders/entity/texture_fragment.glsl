#version 100
precision highp float;

uniform sampler2D texImage;

varying vec2 vTexCoord;
varying float vLightLevel;

void main() {
	vec4 color = texture2D(texImage, vTexCoord);
	gl_FragColor = vec4(color.rgb * vLightLevel, color.a);
}
