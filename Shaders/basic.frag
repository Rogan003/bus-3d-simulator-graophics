#version 330 core
out vec4 FragColor;

in vec3 chNormal;  
in vec3 chFragPos;  
in vec2 chUV;
  
uniform vec3 uLightPos; 
uniform vec3 uViewPos; 
uniform vec3 uLightColor;
uniform float uLightIntensity;

uniform sampler2D uDiffMap1;

void main()
{    
    // Proporcije ambijentalnog, difuznog i spekularnog osvetljenja
    float ambientStrength = 0.2;
    float specularStrength = 0.5;

    // Ambijentalna komponenta
    vec3 ambient = ambientStrength * uLightColor * uLightIntensity;
  	
    // Difuzna komponenta 
    vec3 norm = normalize(chNormal);
    vec3 lightDir = normalize(uLightPos - chFragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * uLightColor * uLightIntensity;
    
    // Spekularna komponenta (Phong)
    vec3 viewDir = normalize(uViewPos - chFragPos);
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = specularStrength * spec * uLightColor * uLightIntensity;  

    // Slabljenje svetlosti (Attenuation) - da bi izgledalo "prikladnije"
    float distance = length(uLightPos - chFragPos);
    float attenuation = 1.0 / (1.0 + 0.045 * distance + 0.0075 * (distance * distance));

    vec3 result = (ambient + diffuse + specular) * attenuation;

    FragColor = texture(uDiffMap1, chUV) * vec4(result, 1.0);
}

