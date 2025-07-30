uniform float time;
out vec4 FragColor;

void main()
{
    float glow = sin(time * 4.0) * 0.2 + 0.3;
    vec3 color = mix(vec3(0.2, 0.5, 1.0), vec3(0.0, 0.9, 1.0), glow);
    FragColor = vec4(color, 1.0);
}