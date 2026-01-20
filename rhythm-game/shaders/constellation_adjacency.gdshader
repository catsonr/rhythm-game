shader_type canvas_item;

// TODO: put these in a uniform group
uniform float t;
uniform float x_offset;
uniform float y_offset;
uniform float aspect_ratio;
uniform float scale; // how many units fit into the height of the shader
uniform vec2 iResolution;
uniform vec4 grid_matrix_vector;

uniform sampler2D adjacency_texture : filter_nearest;
uniform vec2 adjacency_texture_size;

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
	
	/* uv now matches what Observatory expects */

	// flashing highlight on (0, 0)
	float d = 1. - length(uv_std) / 1.5;
	if(d > 0.)
		COLOR = vec4(0, d, d*sin(t)*sin(t), 1.5);
	else
		COLOR = vec4(0);

	/*
		i was unable to get adjacencies rendering through a shader for now, 
		but this is likely where you would start
	*/
	vec2 id_G = floor( Gi*uv_std );
	vec2 id_G_normalized = id_G / adjacency_texture_size;
	vec4 adjacency = texture(adjacency_texture, id_G_normalized);
	

}