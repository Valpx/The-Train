#version 400

in vec3 color;
in vec2 uvs;

layout(location = 0) out vec4 final_col;

uniform int use_texture; // 0 if not. 1 else
uniform sampler2D tex0;

void main()
{
	final_col = vec4(color,1.0);
	if (use_texture == 1) {
		//final_col = vec4(uvs,1.0,1.0);
		final_col = texture(tex0,uvs);
	}
	// final_col = vec4(1.0,0.0,0.0,1.0);
}
