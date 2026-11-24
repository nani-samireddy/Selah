#version 440
layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;
layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    float progress;
    int effect;
} ubuf;
layout(binding = 1) uniform sampler2D outgoing;
layout(binding = 2) uniform sampler2D incoming;

bool inside(vec2 uv) {
    return all(greaterThanEqual(uv, vec2(0))) && all(lessThan(uv, vec2(1)));
}
vec4 bounded(sampler2D source, vec2 uv) {
    return inside(uv) ? texture(source, uv) : vec4(0);
}
vec4 over(vec4 top, vec4 bottom) {
    return top + bottom * (1.0 - top.a);
}
vec4 blurFrame(sampler2D source, vec2 uv, float radius) {
    // Fixed 13-tap kernel. No extra render passes or live video decoders.
    vec2 stepUV = radius / vec2(1920, 1080);
    vec4 result = texture(source, uv) * 0.2;
    for (int i = 0; i < 12; ++i) {
        float angle = float(i) * 0.5235987756;
        result += texture(source, clamp(uv + vec2(cos(angle), sin(angle)) * stepUV,
                          vec2(0), vec2(1))) * (0.8 / 12.0);
    }
    return result;
}
vec2 rotateUV(vec2 uv, float angle, float scale) {
    vec2 centered = (uv - 0.5) * vec2(16.0 / 9.0, 1.0);
    float c = cos(angle), s = sin(angle);
    centered = mat2(c, -s, s, c) * centered / scale;
    return centered / vec2(16.0 / 9.0, 1.0) + 0.5;
}
void main() {
    vec2 uv = qt_TexCoord0;
    float p = clamp(ubuf.progress, 0.0, 1.0);
    vec4 old = texture(outgoing, uv), fresh = texture(incoming, uv), result;
    if (p >= 1.0) { fragColor = fresh * ubuf.qt_Opacity; return; }
    if (p <= 0.0) { fragColor = old * ubuf.qt_Opacity; return; }
    int effect = ubuf.effect;
    if (effect == 0) {
        result = mix(old, fresh, p);
    } else if (effect <= 12) {
        int direction = (effect - 1) % 4;
        vec2 axis = direction == 0 ? vec2(1, 0) : direction == 1 ? vec2(-1, 0) :
                    direction == 2 ? vec2(0, 1) : vec2(0, -1);
        float coordinate = direction == 0 ? 1.0 - uv.x : direction == 1 ? uv.x :
                           direction == 2 ? 1.0 - uv.y : uv.y;
        if (effect <= 4) {
            result = coordinate < p ? fresh : old;
        } else if (effect <= 8) {
            vec2 incomingUV = uv - axis * (1.0 - p);
            result = inside(incomingUV) ? texture(incoming, incomingUV) : bounded(outgoing, uv + axis * p);
        } else {
            vec2 incomingUV = uv - axis * (1.0 - p);
            result = inside(incomingUV) ? texture(incoming, incomingUV) : old;
        }
    } else if (effect == 13 || effect == 14) {
        // Preserve alpha: transparent overlays do not gain an opaque flash rectangle.
        vec3 tint = effect == 13 ? vec3(0) : vec3(1);
        result = p < 0.5 ? mix(old, vec4(tint * old.a, old.a), p * 2.0) :
                           mix(vec4(tint * fresh.a, fresh.a), fresh, p * 2.0 - 1.0);
    } else if (effect == 15 || effect == 16) {
        bool zoomIn = effect == 15;
        float scaleOld = zoomIn ? 1.0 + p : 1.0 - p * 0.7;
        float scaleNew = zoomIn ? 0.3 + p * 0.7 : 2.0 - p;
        vec4 transformedOld = bounded(outgoing, (uv - 0.5) / scaleOld + 0.5);
        vec4 transformedNew = bounded(incoming, (uv - 0.5) / scaleNew + 0.5);
        result = zoomIn ? over(transformedNew * p, transformedOld) :
                          over(transformedOld * (1.0 - p), transformedNew);
    } else if (effect == 17) {
        // Overscan the rotating rectangles so opaque slides never expose empty corners.
        float angle = p * 3.14159265;
        float scale = abs(cos(angle)) + abs(sin(angle)) * (16.0 / 9.0) + 0.001;
        result = mix(bounded(outgoing, rotateUV(uv, p * 3.14159265, scale)),
                     bounded(incoming, rotateUV(uv, (p - 1.0) * 3.14159265, scale)), p);
    } else if (effect == 18) {
        float radius = sin(p * 3.14159265) * 36.0;
        result = mix(blurFrame(outgoing, uv, radius), blurFrame(incoming, uv, radius), p);
    } else if (effect == 19 || effect == 20) {
        float distance = length((uv - 0.5) * vec2(16.0 / 9.0, 1.0)) / 1.02;
        float mask = effect == 19 ? step(distance, p) : step(1.0 - p, distance);
        result = mix(old, fresh, mask);
    } else {
        // Staggered horizontal strokes, with deterministic bristle edges.
        float band = floor(uv.y * 7.0);
        float bristles = sin(uv.y * 1300.0) * 0.012 + sin(uv.y * 367.0) * 0.018;
        float x = mod(band, 2.0) < 1.0 ? uv.x : 1.0 - uv.x;
        float arrival = (band + clamp(x + bristles, 0.0, 1.0) * 2.0) / 9.0;
        result = mix(old, fresh, step(arrival, p));
    }
    fragColor = result * ubuf.qt_Opacity;
}
