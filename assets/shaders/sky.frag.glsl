in vec4 point;
uniform vec3 eye;
out vec4 frag;
float h(vec2 p){return fract(sin(dot(p,vec2(127.1,311.7)))*43758.5453);}
float noise(vec2 p){vec2 i=floor(p),f=fract(p);f=f*f*(3.-2.*f);return mix(mix(h(i),h(i+vec2(1,0)),f.x),mix(h(i+vec2(0,1)),h(i+1.),f.x),f.y);}
void main(){
    vec4 w=point;
    vec3 ray=normalize(w.xyz/w.w-eye);
    float alt=max(ray.y,0.);
    vec3 sky=mix(vec3(.86,.76,.59),vec3(.32,.53,.60),smoothstep(-.01,.72,alt));
    vec3 sun=normalize(vec3(.62,.34,-.88));
    float d=distance(ray,sun);
    sky+=vec3(.31,.19,.085)*exp(-d*6.);
    vec2 p=ray.xz/(alt+.12)*vec2(1.6,3.5);
    float bands=noise(p)*.52+noise(p*2.1)*.30+noise(p*4.4)*.18;
    float cloud=smoothstep(.45,.69,bands)*smoothstep(.02,.17,alt);
    sky=mix(sky,vec3(.84,.80,.68)+.06*bands,cloud*.69);
    sky=mix(sky,vec3(1.,.90,.65),(1.-smoothstep(.032,.038,d)));
    frag=vec4(sky,1.);
}
