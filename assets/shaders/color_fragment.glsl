precision highp float;

uniform vec4 uColor;

varying vec4 vColor;

void main() {
	gl_FragColor = vColor * uColor;
}
