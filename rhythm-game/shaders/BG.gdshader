shader_type canvas_item;

uniform float t;

const float speed = 1./64.;
const float scale = 15.;

const vec3 dark_blue  = vec3(39, 97, 255)/255.;
const vec3 light_blue = vec3(57, 143, 255)/255.;

vec3 tile(vec2 uv, vec2 id)
{
	if(int(id.x+id.y) % 2 == 0) return dark_blue;
	
	return light_blue;
}

void fragment() {
	vec2 uv = vec2(UV.x, 1. - UV.y);
	
	uv *= scale;
	uv += t/8.;

	const float a = 3.1415/6.;
	const mat2 R = mat2(
		vec2(cos(a), sin(a)),
		vec2(-sin(a), cos(a))
	);
	uv *= R;
	
	vec2 id = floor(uv);
	uv = fract(uv);

	COLOR = vec4(tile(uv, id), 1); // draw checkerboard
	COLOR += UV.y/2.; // make lighter at bottom
}