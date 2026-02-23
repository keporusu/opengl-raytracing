#version 330 core
out vec4 FragColor;
in vec2 TexCoord;

uniform sampler2D accumTexture;
uniform int ray_sample_number;

void main(){


    vec3 color=texture(accumTexture,TexCoord).xyz;
    
    //蓄積した色をサンプルの数で割る
    color = color / float(max(ray_sample_number, 1));

    FragColor=vec4(color,1.0);
}