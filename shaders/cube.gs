layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;
in vec4 vs_color[3];
in vec3 vs_uvw[3];
uniform mat4 projection;
flat out vec4 normal;
const vec4 light_position = vec4(300,1000,-300,0);
out vec4 light_direction;
out vec4 vertex_pos;
out vec3 uvw;
out vec4 gs_color;

out vec3 barycentric;
const vec3[] barycentric_vals = vec3[3](vec3(1,0,0), vec3(0,1,0), vec3(0,0,1));

void main() {
	int n = 0;
	vec3 ab = gl_in[1].gl_Position.xyz - gl_in[0].gl_Position.xyz;
	vec3 bc = gl_in[2].gl_Position.xyz - gl_in[1].gl_Position.xyz;
	normal = vec4(cross(ab, bc), 0.);
	for (n = 0; n < gl_in.length(); n++) {
		light_direction = light_position - gl_in[n].gl_Position;
		vertex_pos= gl_in[n].gl_Position;
		gl_Position = projection * gl_in[n].gl_Position;
		barycentric = barycentric_vals[n];
        gs_color = vs_color[n];
		uvw = vs_uvw[n];
		EmitVertex();
	}
	EndPrimitive();
}
