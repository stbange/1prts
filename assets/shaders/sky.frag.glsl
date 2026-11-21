in vec4 point;
uniform vec3 eye;
uniform int nightSky;
out vec4 frag;
float h(vec2 p){vec3 q=fract(p.xyx*vec3(.1031,.11369,.13787));q+=dot(q,q.yzx+19.19);return fract((q.x+q.y)*q.z);}
float noise(vec2 p){vec2 i=floor(p),f=fract(p);f=f*f*(3.-2.*f);return mix(mix(h(i),h(i+vec2(1,0)),f.x),mix(h(i+vec2(0,1)),h(i+1.),f.x),f.y);}
void main(){
    vec4 w=point;
    vec3 ray=normalize(w.xyz/w.w-eye);
    float alt=max(ray.y,0.);
    if(nightSky!=0){
        vec2 starMap=vec2(atan(ray.z,ray.x),asin(clamp(ray.y,-1.,1.)))*145.;
        float star=step(.987,h(floor(starMap)))*(1.-smoothstep(.05,.32,length(fract(starMap)-.5)));
        vec3 sky=mix(vec3(.045,.085,.14),vec3(.14,.23,.33),1.-alt)+vec3(star);
        if(nightSky==2){
            sky*=vec3(.65,.70,.90);
            float limb=length(ray-normalize(vec3(-.42,.31,-.86)));
            sky=mix(sky,vec3(.29,.49,.58)+.13*sin(ray.y*61.),1.-smoothstep(.222,.228,limb));
        }
        frag=vec4(sky,1.);return;
    }
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
