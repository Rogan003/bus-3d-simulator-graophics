#version 330 core

in vec2 chTex;
out vec4 outCol;

uniform sampler2D signatureTex;

void main()
{
    outCol = texture(signatureTex, chTex);
}