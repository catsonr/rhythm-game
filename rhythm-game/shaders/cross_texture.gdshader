shader_type canvas_item;
render_mode blend_mul;

uniform vec2 size  = vec2(800, 600);

void fragment()
{
	vec2 uv = vec2(UV.x, 1.-UV.y); // [0, 1]
	uv = (uv - 0.5) * (size / size.y);

    uv *= 40.;
    uv += TIME/4.;
    
    float a = 0.2, c = cos(a), s = sin(a);
    mat2 R = mat2(vec2(c, s), vec2(-s, c));
    
    uv *= R;
    
    uv = fract(uv);
    uv -= 0.5;  

    COLOR = 1.6-vec4( length(uv) );
}