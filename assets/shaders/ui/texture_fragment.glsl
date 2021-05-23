#version 100
precision highp float;

uniform sampler2D texImage;

varying vec2 vTexCoord;

void main() {
	gl_FragColor = texture2D(texImage, vTexCoord);
}
