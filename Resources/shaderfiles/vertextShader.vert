#version 440 core
layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;
layout(location = 2) in vec2 textCoor;

out vec4 vertexColor;
out vec2 vertexTexture;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
gl_Position = projection * view * model * vec4(position, 1.0f);
vertexColor=  color;
vertexTexture = textCoor; //reference incoming texture data
}
