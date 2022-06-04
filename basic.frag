#version 460 core

const uint voxelsInBlock = (16u*16u*16u);

layout(location = 0) out vec4 color;

layout (std430, binding=2) buffer voxelBuffer
{ 
	uint[16][16][16] offsetBlock;
	uint numBlocks;
	uint[] voxels;
};

uniform float u_Time;
uniform vec3 u_CameraPosition;


const vec2 res = vec2(800,600);

in float id;

in vec3 pos;
in vec3 voxelPos;

float s01(float x)
{
	return sin(x)*0.5+0.5;
}
int getVoxelIndex(int x, int y, int z)
{
	return x + 16 * y + 256 * z;
}

uint unpackPacket(uint packet, uint index)
{
	uint byteMask = 0xff << (index*8);
	return (packet & byteMask) >> (index*8);
}

uint packPacket(uint data[4])
{
	uint r = data[0];
	r |= (data[1] << 8);
	r |= (data[2] << 16);
	r |= (data[3] << 24);

	return r;
}

uint fetchVoxel(ivec3 p)
{
	uint offset = offsetBlock[p.x/16][p.y/16][p.z/16];
	uint index = voxelsInBlock * offset + getVoxelIndex(p.x % 16, p.y % 16, p.z % 16);
	return unpackPacket( voxels[index/4],index%4);
}
void main()
{
	float t = u_Time;
	float s = sin(t) * 0.5 + 0.5;
	vec2 uv = gl_FragCoord.xy / res;
	color = vec4(floor(pos+0.001)/64.0,1.0);
	int test = int(pos.x)%16;
	uint voxelOffset = offsetBlock[int(round(voxelPos.x))][2][4];
	color.rgb = round(mod(5*voxelPos,16.0))/15.0;

	vec3 ro = pos * 16.0;
	vec3 p = ro;
	vec3 rd = normalize(u_CameraPosition-pos);
	color.rgb = vec3(fetchVoxel(ivec3(ro + rd * 0.01)));


	//vec3 subvPos = pos-round(pos);
	//uint voxelID = voxels[voxelOffset*4096];
	////float i = float(voxelID)/255.0;
	//color = vec4(i,i,i,1);

};
