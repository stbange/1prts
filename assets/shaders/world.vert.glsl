layout(location=0) in vec3 position;
layout(location=1) in vec3 normal;
layout(location=2) in vec4 color;
uniform mat4 vp;
uniform vec4 objectPose;
uniform vec3 eye;
uniform int shadowPass;
uniform int compactLivery;
out vec3 world;
out vec3 norm;
out vec3 paint;
out float rangeToEye;
void main(){
    float c=cos(objectPose.w), s=sin(objectPose.w);
    mat3 turn=mat3(c,0.,s, 0.,1.,0., -s,0.,c);
    world=turn*position+objectPose.xyz;
    if(shadowPass!=0){ world.xz+=world.y*vec2(.62,-.32);world.y=.073; }
    norm=turn*normal; paint=color.rgb;
    if(compactLivery!=0 && paint.b>paint.r*1.15 && paint.g>paint.r*1.15)
        paint=mix(paint,vec3(.46,.31,.40),.76);
    rangeToEye=length(world-eye);
    gl_Position=vp*vec4(world,1.);
}
