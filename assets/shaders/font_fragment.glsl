#version 100
precision highp float;

uniform sampler2D texImage;

varying vec2 vTexCoord;
varying vec4 vColor;

void main() {
	if (texture2D(texImage, vTexCoord).r < 0.5) {
		discard;
	}
	gl_FragColor = vColor;
}
