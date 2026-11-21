layout(location=0) in vec2 position;
layout(location=1) in vec4 color;
uniform vec2 extent;
uniform mat4 wristProjection;
uniform int tracked;
out vec4 ink;
void main() {
    ink=color;
    vec2 p=position/extent*vec2(2.,-2.)+vec2(-1.,1.);
    gl_Position=tracked!=0 ? wristProjection*vec4(p*vec2(.26,.22),0.,1.) : vec4(p,0.,1.);
}
