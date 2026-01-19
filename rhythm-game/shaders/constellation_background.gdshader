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

// returns the distance to the nearest lattice point of a REASONABLE basis
// where a reasonable lattice is one that is roughly orthogonal
// TODO: this constraint could possible be fixed with: https://en.wikipedia.org/wiki/Lattice_reduction
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

// for some distance d, if it is close enough to 0, it is 'on' (return 1.) else it's 'off' (return 0.)
const float on_radius = 0.75;
float on(float d) { return smoothstep(10./iResolution.y, 0., d - on_radius); }

void fragment() {
	// construct 2x2 matrix G from vec4 grid_matrix_vector, whose components would be row-major (hence r, b, g, a)
	// this hack is needed since 1) godot shader does not accept uniform matricies (!?)
	// and 2) godot shader lang matrices are constructed through column-major vectors
	mat2 G = mat2(
		vec2(grid_matrix_vector.r, grid_matrix_vector.b),
		vec2(grid_matrix_vector.g, grid_matrix_vector.a)
	), Gi = inverse(G);

	// match coordinate plane to that of Observatory
	vec2 uv_std = vec2(UV.x, UV.y);
	uv_std -= 0.5; // move origin to center of screen
	uv_std.x *= aspect_ratio;
	uv_std *= scale;
	uv_std.x += x_offset;
	uv_std.y += y_offset;
	
	// find nearest lattice point (again, assuming a reasonable basis)
	vec2 uv_G = Gi * uv_std;
	vec2 G_id = floor(uv_G);
	
	// draw the points! (with some chromatic aberration distance a)
	float a = 0.05*sin(t/8. + G_id.x);
	COLOR = vec4(
		on(lattice( uv_std, G_id, G )),
		on(lattice( uv_std + vec2(a, -a), G_id, G )),
		on(lattice( uv_std - vec2(a, -a), G_id, G )),
	1);
	
	// red bg  -> larger id.x
	COLOR.r += G_id.x/10.;
	// blue bg -> larger id.y
	COLOR.b += G_id.y/10.;
	
	// draw grid lines for standard basis (debug)
	uv_std = fract(uv_std);
	//if(uv_std.x <= 0.015 || uv_std.y <= 0.015) COLOR.rgb = vec3(.1);
}