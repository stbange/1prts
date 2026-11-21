out vec4 point;
uniform mat4 sceneVP;
void main(){
    vec2 p=vec2((gl_VertexID==1)?3.:-1.,(gl_VertexID==2)?3.:-1.);
    point=inverse(sceneVP)*vec4(p,.8,1.);gl_Position=vec4(p,.9999,1.);
}
