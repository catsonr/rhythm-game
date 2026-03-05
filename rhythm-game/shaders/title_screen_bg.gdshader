shader_type canvas_item;
render_mode blend_mul;

uniform vec2 size  = vec2(800, 600);
uniform vec2 mouse = vec2(0);

//#define uv here!
#define t (TIME/4.)

// the function to be graphed
// NOTE: tan looks like quarter notes!
float f(float x) { return sin(x + t); }

// draws the function f
float graph(vec2 uv) {
    float d = abs( f(uv.x+t) - uv.y ); // the distance from uv to the graph

    return d <= 1. ? 1. : 0.; // return 1 if d is small enough
}

void fragment()
{
	vec2 uv = vec2(UV.x, 1.-UV.y); // [0, 1]
	uv = (uv - 0.5) * (size / size.y);

	uv.y *= 3.;
	uv.x *= 2.;

	uv += mouse * 0.5;
	
	uv *= mat2(
		vec2(1, 0),
		vec2(0.5, 1)
	);

	float l = cos(t)*cos(t)*sin(t)*.5;

	COLOR = vec4(
		graph(uv),
		graph(uv + l + mouse.x*.2),
		graph(uv - l + mouse.y*.2),
	1);
}