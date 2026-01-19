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

float lattice(vec2 uv_std, vec2 id_G, mat2 G)
{
	return min(
		min(
			length(uv_std - G*id_G),
			length(uv_std - G*(id_G+vec2(1,0)))
		),
		min(
			length(uv_std - G*(id_G+vec2(0,1))),
			length(uv_std - G*(id_G+vec2(1,1)))
		)
	);
}

float on(float d) { return smoothstep(10./iResolution.y, 0., d - 0.75); }

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
	
	vec2 uv_G = Gi * uv_std;
	vec2 G_id = floor(uv_G);
	
	float a = 0.02*sin(t/8. + G_id.x);
	COLOR = vec4(
		on(lattice( uv_std, G_id, G )),
		on(lattice( uv_std + vec2(a, -a), G_id, G )),
		on(lattice( uv_std - vec2(a, -a), G_id, G )),
	1);
	
	uv_std = fract(uv_std);
	//if(uv_std.x <= 0.015 || uv_std.y <= 0.015) COLOR.rgb = vec3(.1);
}