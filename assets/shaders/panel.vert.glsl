layout(location=0) in vec2 position;
layout(location=1) in vec4 color;
uniform vec2 extent;
out vec4 ink;
void main(){ink=color;gl_Position=vec4(position/extent*vec2(2.,-2.)+vec2(-1.,1.),0.,1.);}
