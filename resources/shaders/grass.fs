
#version 330 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;

struct PointLight {
    vec3 position;

    vec3 specular;
    vec3 diffuse;
    vec3 ambient;

    float constant;
    float linear;
    float quadratic;
};

struct DirectionLight{
    vec3 direction;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

};

uniform sampler2D texture1;
uniform PointLight pointLight;
uniform DirectionLight dirlight;
uniform vec3 viewPosition;

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir,vec3 color)
{

    vec3 lightDir = normalize(light.position - fragPos);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    //vec3 reflectDir = reflect(-lightDir, normal);
    vec3 halfway=normalize(lightDir+viewDir);
    float spec = pow(max(dot(normal, halfway), 0.0), 8.0);
    // attenuation
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    // combine results

    vec3 ambient = light.ambient * vec3(color);
    vec3 diffuse = light.diffuse * diff * vec3(color);
    vec3 specular = light.specular * spec * vec3(color);
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;



   // FragColor=vec4(vec3(1.0-shadow)* (diffuse + specular),1.0);
   // FragColor=vec4(vec3(1.0-shadow),1.0);
    //FragColor=vec4(ambient,1.0);

    return (ambient +  (diffuse + specular));
}

void main()
{
    vec3 normal = normalize(Normal);
    vec3 viewDir = normalize(viewPosition - FragPos);

    vec4 texColor = texture(texture1, TexCoords);
    if(texColor.a < 0.1)
            discard;
    vec3 resultat=CalcPointLight(pointLight,normal,FragPos,viewDir,texColor.rgb);

//TODO calculate tex*light
   // FragColor=texColor;
    FragColor = vec4(resultat,1.0);
}