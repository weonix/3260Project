#version 430

out vec4 daColor;
//in vec3 theColor;

in vec2 UV;
in vec3 normalWorld;
in vec3 vertexPositionWorld;

uniform sampler2D myTextureSampler;
uniform vec4 ambientLight;
uniform vec4 diffuseLight;
uniform vec4 specularLight;
uniform vec3 lightPositionWorld;
uniform vec4 eyePositionWorld;

void main()
{
	vec3 color = texture( myTextureSampler, UV ).rgb;
	//daColor = vec4(color,1.0);
	vec3 lightVectorWorld = normalize(lightPositionWorld - vertexPositionWorld);
	float brightness = dot(lightVectorWorld, normalize(normalWorld));
	float distance = dot((lightPositionWorld - vertexPositionWorld),(lightPositionWorld - vertexPositionWorld));
	vec4 diffuseLightFin = vec4(brightness, brightness, brightness, 1.0) * diffuseLight;// * 1/distance * 2;


	vec4 MaterialAmbientColor = vec4(texture(myTextureSampler, UV ).rgb, 1.0);
	vec4 MaterialDiffuseColor = vec4(texture(myTextureSampler, UV ).rgb, 1.0);
	vec4 MaterialSpecularColor = vec4(0.7,0.7,0.7, 1.0);

	vec3 reflectedLightVectorWorld = reflect(-lightVectorWorld, normalWorld);
	vec3 eyeVectorWorld = normalize(eyePositionWorld.xyz - vertexPositionWorld);
	float s = clamp(dot(reflectedLightVectorWorld, eyeVectorWorld),0,1) ;
	s = pow(s, 20);
	vec4 specularLightFin = vec4(s,s,s,1) * specularLight;

	daColor = MaterialAmbientColor * ambientLight +  clamp(MaterialDiffuseColor* diffuseLightFin,0,1) + MaterialSpecularColor * specularLightFin;

	//daColor = ambientLight + specularLightFin;
	//daColor = vec4(normalize (eyeVectorWorld),1.0f);
	//daColor = MaterialSpecularColor * specularLightFin;//vec4(s,s,s,1);
}