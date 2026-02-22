#version 330 core
out vec4 FragColor;
in vec2 TexCoord;

uniform sampler2D accumTexture;

void main(){

    //vec4 texColor1=texture(texture1,TexCoord);
    //vec4 texColor2=texture(texture2,TexCoord);
    vec3 color=texture(accumTexture,TexCoord).xyz;

    FragColor=vec4(color,1.0);
}