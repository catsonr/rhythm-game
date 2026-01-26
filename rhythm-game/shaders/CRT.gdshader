/*
this shader was taken directly from:
https://www.shadertoy.com/view/Ms23DR
and converted to .gdscript with gemini -- i didn't write a single line of this ¯\_(ツ)_/¯ 
*/

shader_type canvas_item;

// This lets us access the pixels of the game behind the ColorRect
uniform sampler2D screen_texture : hint_screen_texture, filter_linear_mipmap;

// Settings you can tweak in the Inspector
uniform float curvature_strength : hint_range(0.0, 10.0) = 5.0;
uniform float scan_intensity : hint_range(0.0, 1.0) = 0.35;
uniform float vignette_opacity : hint_range(0.0, 1.0) = 0.3;
uniform float brightness : hint_range(0.0, 5.0) = 2.8;
uniform float grille_opacity : hint_range(0.0, 1.0) = 0.15;
uniform float resolution : hint_range(100.0, 2000.0) = 680.0;

// Helper function to curve the screen (CRT Bulge)
vec2 curve(vec2 uv) {
    uv = (uv - 0.5) * 2.0;
    uv *= 1.1;    
    uv.x *= 1.0 + pow((abs(uv.y) / curvature_strength), 2.0);
    uv.y *= 1.0 + pow((abs(uv.x) / 4.0), 2.0); // Note: 4.0 matches the original code's aspect ratio tweak
    uv  = (uv / 2.0) + 0.5;
    uv =  uv * 0.92 + 0.04;
    return uv;
}

void fragment() {
    // 1. Get standard screen UVs
    vec2 q = SCREEN_UV;
    
    // 2. Apply CRT Curve
    vec2 uv = curve(q);
    
    // 3. Define the source color variable
    vec3 col;
    
    // 4. Chromatic Aberration & Wiggle
    // 'TIME' replaces 'iTime'
    float x = sin(0.3 * TIME + uv.y * 21.0) * sin(0.7 * TIME + uv.y * 29.0) * sin(0.3 + 0.33 * TIME + uv.y * 31.0) * 0.0017;

    // Read the Red, Green, and Blue channels separately with slight offsets (chromatic aberration)
    // Note: We use 'screen_texture' instead of 'iChannel0'
    col.r = texture(screen_texture, vec2(x + uv.x + 0.001, uv.y + 0.001)).x + 0.05;
    col.g = texture(screen_texture, vec2(x + uv.x + 0.000, uv.y - 0.002)).y + 0.05;
    col.b = texture(screen_texture, vec2(x + uv.x - 0.002, uv.y + 0.000)).z + 0.05;
    
    // Add "Ghosting" / Bloom-like bleed
    col.r += 0.08 * texture(screen_texture, 0.75 * vec2(x + 0.025, -0.027) + vec2(uv.x + 0.001, uv.y + 0.001)).x;
    col.g += 0.05 * texture(screen_texture, 0.75 * vec2(x + -0.022, -0.02) + vec2(uv.x + 0.000, uv.y - 0.002)).y;
    col.b += 0.08 * texture(screen_texture, 0.75 * vec2(x + -0.02, -0.018) + vec2(uv.x - 0.002, uv.y + 0.000)).z;

    // 5. Contrast / Tone Mapping
    col = clamp(col * 0.6 + 0.4 * col * col * 1.0, 0.0, 1.0);

    // 6. Vignette (Dark corners)
    float vig = (0.0 + 1.0 * 16.0 * uv.x * uv.y * (1.0 - uv.x) * (1.0 - uv.y));
    col *= vec3(pow(vig, vignette_opacity));

    // 7. Color Tining & Brightness
    col *= vec3(0.95, 1.05, 0.95); // Slight green tint
    col *= brightness;

    // 8. Scanlines
    // We use '1.0 / SCREEN_PIXEL_SIZE.y' to get the screen height resolution instead of iResolution.y
    float height = 1.0 / SCREEN_PIXEL_SIZE.y;
    float scans = clamp(0.35 + 0.35 * sin(3.5 * TIME + uv.y * height * 1.5), 0.0, 1.0);
    float s = pow(scans, 1.7);
    col = col * vec3(0.4 + 0.7 * s);

    // 9. Flicker
    col *= 1.0 + 0.01 * sin(110.0 * TIME);
    
    // 10. Black out areas outside the curved screen
    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
        col = vec3(0.0);
    }
    
    // 11. Aperture Grille (Vertical Lines)
    // We use the 'uv' variable instead of FRAGCOORD so the lines curve with the screen.
    // We use a Sine wave instead of 'mod' to prevent ugly Moiré artifacts on different monitors.
    
    float grille = 0.5 + 0.5 * sin(uv.x * resolution * PI);
    
    // Smooth out the effect based on opacity
    // 1.0 means no change, 0.0 means full black at the trough of the sine wave
    col *= mix(1.0, grille, grille_opacity);

    // Output final color
    COLOR = vec4(col, 1.0);
}