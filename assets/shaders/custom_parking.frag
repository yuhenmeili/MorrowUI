#version 460
precision mediump float;
precision mediump int;

in vec3 v_position;
in vec4 v_color;
in vec2 v_texCoord;

uniform sampler2D u_texture;
uniform vec2 u_imageSize;
uniform int u_lightenArray[36];
uniform float u_alpha;

layout(location = 0) out vec4 o_fragColor;
void main()
{
    vec2 currentPosition = u_imageSize/2.0 + v_position.xy;
    float xLength = u_imageSize.x / 4.0;
    float yLength = u_imageSize.y / 9.0;
    int xIndex = int(floor(currentPosition.x / xLength));
    int yIndex = int(floor(currentPosition.y / yLength));
    int flag = u_lightenArray[xIndex + (yIndex * 4)];
    if(flag == 0){
        discard;
    }
    vec4 base_color = texture(u_texture, v_texCoord.st);
    vec4 color = v_color * base_color;
    o_fragColor = color;
    o_fragColor.a *= u_alpha;
}