#version 440
layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;
layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    float progress;
    vec2 panelSize;
    vec2 targetPoint;
} ubuf;
layout(binding = 1) uniform sampler2D source;
void main() {
    fragColor = texture(source, qt_TexCoord0) * ubuf.qt_Opacity *
                (1.0 - smoothstep(0.88, 1.0, ubuf.progress));
}
