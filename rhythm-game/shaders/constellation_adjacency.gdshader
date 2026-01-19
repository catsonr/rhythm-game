shader_type canvas_item;

uniform float t;
uniform float x_offset;
uniform float y_offset;
uniform float aspect_ratio;
uniform float scale; // how many units fit into the height of the shader
uniform vec2 iResolution;
uniform vec4 grid_matrix_vector;

void vertex() {
	// Called for every vertex the material is visible on.
}

void fragment() {
	mat2 G = mat2(
		vec2(grid_matrix_vector.r, grid_matrix_vector.b),
		vec2(grid_matrix_vector.g, grid_matrix_vector.a)
	), Gi = inverse(G);

	vec2 uv_std = vec2(UV.x, UV.y);
	uv_std -= 0.5; // move origin to center of screen
	uv_std.x *= aspect_ratio;
	uv_std *= scale;

	uv_std.x += x_offset;
	uv_std.y += y_offset;

	// flashing highlight on (0, 0)
	float d = 1. - length(uv_std) / 1.5;
	if(d > 0.)
		COLOR = vec4(0, d, d*sin(t)*sin(t), 1.5);
	else
		COLOR = vec4(0);
}