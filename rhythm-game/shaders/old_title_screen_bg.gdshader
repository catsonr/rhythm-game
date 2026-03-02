shader_type canvas_item;

uniform vec2 mouse_dist_from_center = vec2(0); // [-0.5, 0.5]^2

// vec2 variables with the suffix _C are complex numbers; not vectors

vec2 f(vec2 z_C, vec2 c_C)
{
	float a = z_C.x;
	float b = z_C.y;

	return vec2(
		tan(a*a) - sin(b*b) + c_C.x,
		2.*a*b + c_C.y
	);
}

vec3 julia(vec2 uv, vec2 c_C)
{
	const int iterations  = 10;
	const float max_value = 30.;
	const float max_value_squared = max_value * max_value;
	
	vec2 z_C = uv;
	for(int i = 0; i < iterations; i++)
	{
		z_C = f(z_C, c_C);
		if( dot(z_C, z_C) > max_value_squared ) return vec3(float(i)/float(iterations)); // diverged!
	}
	
	return vec3(1);
}

void fragment() {
	vec2 uv = vec2(UV.x, 1.-UV.y);
	uv -= 0.5;
	uv *= 0.12;
	
	float t = TIME/360.;
	vec2 c_C = vec2(sin(t), cos(t))/20. + vec2(0.41, 0);
	
	uv += mouse_dist_from_center * 0.002;
	c_C += mouse_dist_from_center * 0.00002;

	COLOR = vec4(julia(uv, c_C) - 0.4, 1);
}