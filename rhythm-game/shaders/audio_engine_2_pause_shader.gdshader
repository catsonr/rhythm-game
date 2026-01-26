shader_type canvas_item;

uniform vec2 size;
uniform bool playing_track;

void vertex() {
	// Called for every vertex the material is visible on.
}

void fragment() {
	if(playing_track) discard;
	
	vec2 uv = vec2(UV.x, UV.y);
	uv -= 0.5;
	
	uv = abs(uv);
	
	float space = .15;
	float thickness = .12;
	float height_offset = 0.1;
	if( (( abs(space-uv.x) <= thickness ) && (uv.y <= 0.5 - height_offset)) ) discard;

	COLOR = vec4(0, 0, 0, .8);
}