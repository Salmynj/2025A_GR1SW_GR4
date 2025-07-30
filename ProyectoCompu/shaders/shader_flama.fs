#version 330 core
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

out vec4 FragColor;

uniform sampler2D baseTexture;      // Celeste.png
uniform sampler2D glowTexture;      // Azul.png
uniform sampler2D highlightTexture; // Turquesa.png
uniform float time;

void main()
{
    // Base difusa
    vec3 baseColor = texture(baseTexture, TexCoords).rgb*0.2;

    // Glow general animado con Azul.png (parpadeo lento de 4 segundos)
    float pulsate = sin(time * 6.28);
    vec3 glowColor = texture(glowTexture, TexCoords).rgb * pulsate * 4.5;

    // Brillo solo en zonas claras de Turquesa.png
    vec3 highlight = texture(highlightTexture, TexCoords).rgb;
    float intensity = max(max(highlight.r, highlight.g), highlight.b);
    vec3 selectiveGlow = highlight * clamp(intensity - 0.3, 0.0, 1.0) * 6.0;

    vec3 finalColor = baseColor + glowColor + selectiveGlow;
    FragColor = vec4(finalColor, 1.0);
}
