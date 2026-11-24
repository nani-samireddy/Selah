#version 440
layout(location = 0) in vec4 qt_Vertex;
layout(location = 1) in vec2 qt_MultiTexCoord0;
layout(location = 0) out vec2 qt_TexCoord0;
layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    float progress;
    vec2 panelSize;
    vec2 targetPoint;
} ubuf;
void main() {
    qt_TexCoord0 = qt_MultiTexCoord0;
    float p = clamp(ubuf.progress, 0.0, 1.0);
    float row = qt_MultiTexCoord0.y;
    // The edge nearest the toolbar narrows first; the rest follows into the neck.
    float head = smoothstep(0.0, 0.65, p);
    float tail = smoothstep(0.20, 1.0, p);
    float funnel = mix(head, tail, pow(row, 0.85));
    float widthScale = mix(1.0, 0.02, funnel);
    float center = mix(ubuf.panelSize.x * 0.5, ubuf.targetPoint.x, funnel);
    float x = center + (qt_Vertex.x - ubuf.panelSize.x * 0.5) * widthScale;
    float top = mix(0.0, ubuf.targetPoint.y, head);
    float bottom = mix(ubuf.panelSize.y, ubuf.targetPoint.y + 4.0, tail);
    gl_Position = ubuf.qt_Matrix * vec4(x, mix(top, bottom, row), qt_Vertex.z, qt_Vertex.w);
}
