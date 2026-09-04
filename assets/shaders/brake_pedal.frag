
uniform sampler2D u_brakePedalTexture;
uniform sampler2D u_grayTexture;
uniform sampler2D u_whiteTexture;
uniform float u_timeDelta;
uniform float u_duration;
uniform float u_alpha;

layout(location = 0) in vec2 v_texCoord;

const float PI=3.14159265358979323846;
const float ANGLE_OFFSET = PI*0.5;
const float TAU = 6.28318530717958647692;

void main()
{
    vec4 gearsColor = vec4(texture2D(u_brakePedalTexture, v_texCoord.xy).a);
    vec4 grayColor = vec4(texture2D(u_grayTexture, v_texCoord.xy).a);
    vec4 whiteColor = vec4(texture2D(u_whiteTexture, v_texCoord.xy).a);
    vec4 result = gearsColor + grayColor;

    float cd = u_duration * 1000.0;
    float progress = u_timeDelta * 1000.0/cd;
    float endAngle = progress* TAU;
    vec2 uv = v_texCoord - vec2(0.5,0.5);
    float angle = atan(uv.y,uv.x) + ANGLE_OFFSET;
    if( angle < 0.0 ) angle += PI * 2.0;
    //  angle = PI * 2.0 - angle;
    if( angle < endAngle){
        result += whiteColor;
    }
    gl_FragColor = result;
    gl_FragColor.a *= u_alpha;
}
