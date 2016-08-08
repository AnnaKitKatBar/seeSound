#version 330 core 
in vec3 fragNor;
out vec4 color;
in vec3 norm;
in vec3 light;
in vec3 wPos;

uniform vec3 MatAmb;
uniform vec3 MatDif;
uniform vec3 MatSpec;
uniform float shini;
uniform float viewNormals;


void main()
{
   vec3 Il = vec3(1.0);  //light color = white

   //ambience
   vec3 ambient = MatAmb * Il;

   //diffuse
   vec3 diffuse = MatDif * max(dot(normalize(light), normalize(norm)), 0) * Il;

   //specular
   vec3 reflect = -light + (2 * dot(light, norm) * norm);
   vec3 view = vec3(0.0) - wPos;
   vec3 specular = MatSpec * pow(max(dot(normalize(view), normalize(reflect)), 0),shini) * Il;

   //phong
   if(viewNormals == 0.0)
      color = vec4(diffuse + specular + ambient, .8);
   else
      color = vec4(norm, 1.0);

}
