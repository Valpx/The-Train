// Vertex Shader
#version 120

varying vec4 pos_sommet;
varying vec3 nml_sommet;

void main() {
    gl_Position  =  gl_ProjectionMatrix * gl_ModelViewMatrix * gl_Vertex;
    pos_sommet = gl_Vertex;
    nml_sommet = gl_Normal;
}

