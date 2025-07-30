#version 330 core
out vec4 FragColor;

uniform float time;

// Emisión que parpadea (puedes cambiar estos colores)
vec3 onColor = vec3(0.35, 0.05, 0.5); // Luz encendida morado
vec3 offColor = vec3(0.05, 0.05, 0.05); // Luz apagada (casi negra)

void main()
{
    // Usa sin para parpadeo suave, o step/mod para parpadeo duro
    float blinking = abs(sin(time * 3.0)); // Más rápido
    vec3 finalColor = mix(offColor, onColor, blinking);

    FragColor = vec4(finalColor, 1.0);
}
