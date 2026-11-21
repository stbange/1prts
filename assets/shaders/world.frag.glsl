in vec3 world;
in vec3 norm;
in vec3 paint;
in float rangeToEye;
uniform int shadowPass;
uniform int nightLight;
out vec4 frag;
float hash(vec3 p){ p=fract(p*vec3(.1031,.11369,.13787)); p+=dot(p,p.yzx+19.19); return fract((p.x+p.y)*p.z); }
void main(){
    vec3 n=normalize(norm);
    float sun=max(dot(n,normalize(vec3(-.54,.74,.40))),0.);
    float top=.5+.5*n.y;
    vec3 light=mix(vec3(.56,.64,.66),vec3(.73,.78,.77),top)+vec3(.44,.35,.23)*sun;
    float grit=(hash(floor(world*6.))-.5)*.052 + (hash(floor(world*1.3))-.5)*.027;
    float fog=1.-exp(-rangeToEye*.0032);
    vec3 shaded=paint*light+grit*(1.-smoothstep(32.,140.,rangeToEye));
    if(shadowPass!=0) shaded=vec3(.307,.335,.299)+grit*.65;
    vec3 horizon=vec3(.73,.70,.59);
    if(nightLight!=0){
        vec3 moon=nightLight==2 ? vec3(.65,.78,.97) : vec3(.38,.52,.73);
        bool lamp=paint.r>.90 && paint.g>.68 && paint.b<.59 && shadowPass==0;
        shaded=lamp ? paint*1.12 : shaded*moon;
        horizon=nightLight==2 ? vec3(.065,.09,.16) : vec3(.12,.18,.26);
        if(nightLight==2)fog*=.48;
    }
    frag=vec4(mix(shaded,horizon,fog),1.);
}
