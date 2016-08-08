#version  330 core
layout(location = 0) in vec4 vertPos;
layout(location = 1) in vec3 vertNor;
uniform mat4 P;
uniform mat4 M;
uniform mat4 V;
out vec3 fragNor;
uniform float lightXpos;
uniform float lightYpos;

out vec3 norm;
out vec3 light;
out vec3 wPos;


void main() {
   vec3 lightSrc = vec3(lightXpos, lightYpos + 5, 15.0);

	gl_Position = P * V * M * vertPos;
	fragNor = normalize((M * vec4(vertNor, 0.0)).xyz);

   //vec3 light1 = lightSrc - (M * vertPos).xyz;   //for point light
   vec3 light1 = lightSrc;   //directional light
 
   //phong
   norm = normalize((M * vec4(vertNor, 0.0)).xyz);
   light = light1;
   wPos = (M * vertPos).xyz;
}
