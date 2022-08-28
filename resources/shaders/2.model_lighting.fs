#version 330 core
out vec4 FragColor;

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

uniform samplerCube depthMap;

struct Material {
    sampler2D texture_diffuse1;
    sampler2D texture_specular1;

    float shininess;
};
in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;

uniform PointLight pointLight;
uniform DirectionLight dirlight;
uniform Material material;

uniform bool shadows;
uniform float far_plane;

uniform vec3 viewPosition;
// calculates the color when using a point light.
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir,float shadow)
{

    vec3 lightDir = normalize(light.position - fragPos);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    //vec3 reflectDir = reflect(-lightDir, normal);
    vec3 halfway=normalize(lightDir+viewDir);
    float spec = pow(max(dot(normal, halfway), 0.0), material.shininess);
    // attenuation
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    // combine results

    vec3 ambient = light.ambient * vec3(texture(material.texture_diffuse1, TexCoords));
    vec3 diffuse = light.diffuse * diff * vec3(texture(material.texture_diffuse1, TexCoords));
    vec3 specular = light.specular * spec * vec3(texture(material.texture_specular1, TexCoords).xxx);
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;



   // FragColor=vec4(vec3(1.0-shadow)* (diffuse + specular),1.0);
   // FragColor=vec4(vec3(1.0-shadow),1.0);
    //FragColor=vec4(ambient,1.0);
   if(shadows)return (ambient +diffuse+specular);
   else return (ambient + (1.0-shadow)* (diffuse + specular));
}

vec3 calculateDirLight(DirectionLight light,vec3 normal,vec3 fragPos,vec3 viewDir){
    vec3 lightdir=normalize(-light.direction);
    float diff = max(dot(normal, lightdir), 0.0);


    vec3 halfway=normalize(lightdir+viewDir);
    float spec = pow(max(dot(normal, halfway), 0.0), material.shininess);


    vec3 ambient = light.ambient * vec3(texture(material.texture_diffuse1, TexCoords));
    vec3 diffuse = light.diffuse * diff * vec3(texture(material.texture_diffuse1, TexCoords));
    vec3 specular = light.specular * spec * vec3(texture(material.texture_specular1, TexCoords).xxx);

    return (ambient+diffuse+specular);

}

float calcShadow(PointLight light,vec3 fragPos){
           vec3 fragToLight = fragPos - light.position;
           float closestDepth = texture(depthMap, fragToLight).r;
           closestDepth *= far_plane;
           float currentDepth = length(fragToLight);
           float bias = 0.05;
           float shadow = currentDepth -  bias > closestDepth ? 1.0 : 0.0;
           // display closestDepth as debug (to visualize depth cubemap)
           FragColor = vec4(vec3(closestDepth / far_plane), 1.0);

           return shadow;
}


void main()
{
    vec3 normal = normalize(Normal);
    vec3 viewDir = normalize(viewPosition - FragPos);
    float shadow = calcShadow(pointLight,FragPos) ;
    vec3 result = CalcPointLight(pointLight, normal, FragPos, viewDir,shadow);
    result+=calculateDirLight(dirlight,normal,FragPos,viewDir);
    FragColor = vec4(result, 1.0);
}