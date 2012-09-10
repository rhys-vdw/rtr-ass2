// vertex shader for per-pixel lighting
// assumes single directional light

#define M_PI 3.1415926535897932384626433832795

varying vec3 eye;
varying vec3 normal;

//uniform float R;
//uniform float r;

void main(void)
{
	float R = 2.0;
	float r = 0.5;

	float u = gl_Vertex.x * 2.0 * M_PI;
	float v = gl_Vertex.y * 2.0 * M_PI;

	// generate torus
	vec3 normal = vec3(
		cos(u) * cos(v),
		sin(u) * cos(v),
		sin(v));

	vec4 vertex = vec4(
		(R + r * cos(v)) * cos(u),
		(R + r * cos(v)) * sin(u),
		r * sin(v),
		1);

	// TODO: pass position and normal to fragment shaders
	eye = normalize(vec3(gl_ModelViewMatrix * vertex));
	normal = normalize(vec3(gl_NormalMatrix * normal));

	// apply matrix transforms to vertex position
	gl_Position = gl_ModelViewProjectionMatrix * vertex;
}

