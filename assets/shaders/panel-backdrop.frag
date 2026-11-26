#version 440
layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;
layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    vec4 sampleRect;
    vec2 panelSize;
    vec4 tint;
    float cornerRadius;
    float saturation;
} ubuf;
layout(binding = 1) uniform sampler2D backdrop;
void main() {
    vec2 uv = ubuf.sampleRect.xy + qt_TexCoord0 * ubuf.sampleRect.zw;
    vec4 softened = texture(backdrop, clamp(uv, vec2(0), vec2(1)));
    // Saturate the backdrop before tinting, preserving premultiplied alpha.
    vec3 rgb = softened.a > 0.00001 ? softened.rgb / softened.a : vec3(0);
    float luminance = dot(rgb, vec3(0.2126, 0.7152, 0.0722));
    softened.rgb = clamp(mix(vec3(luminance), rgb, ubuf.saturation), 0.0, 1.0) * softened.a;
    // The shared blurred scene is behind the tint; controls render sharply above it.
    vec4 surface = ubuf.tint + softened * (1.0 - ubuf.tint.a);
    vec2 q = abs((qt_TexCoord0 - 0.5) * ubuf.panelSize) - ubuf.panelSize * 0.5 + ubuf.cornerRadius;
    vec2 corner = max(q, vec2(0));
    // Match the squircle surface: fourth-power corners instead of circular arcs.
    float distance = pow(pow(corner.x, 4.0) + pow(corner.y, 4.0), 0.25)
        + min(max(q.x, q.y), 0.0) - ubuf.cornerRadius;
    float mask = 1.0 - smoothstep(-0.7, 0.7, distance);
    fragColor = surface * mask * ubuf.qt_Opacity;
}
