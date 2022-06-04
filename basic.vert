#version 460 core
		
layout(location = 0) in vec3 position;
layout(location = 1) in uvec3 offset;

uniform mat4 u_MVP;

out float id;
out vec3 pos;
out vec3 voxelPos;
void main()
{
	id = float(gl_InstanceID.x);
	vec2 o = vec2(floor(id/10.0),mod(id,10.0));
	pos = position+vec3(offset);
	voxelPos = vec3(offset%16);
	gl_Position = u_MVP*vec4(position+vec3(offset),1);
}