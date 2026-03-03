#version 450

/*====================================================================================================================================
                                                  FILAMENT_FULLSCREEN.VERT
====================================================================================================================================*/
// 🧩 Fullscreen triangle — no vertex buffer, generates a triangle covering the screen.

layout(location = 0) out vec2 fragUV;

void main()
{
    // ⚙️ Fullscreen triangle trick: 3 vertices covering [-1,1] clip space
    //    Vertex 0: (-1, -1)  UV (0, 0)
    //    Vertex 1: ( 3, -1)  UV (2, 0)
    //    Vertex 2: (-1,  3)  UV (0, 2)
    fragUV = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4(fragUV * 2.0 - 1.0, 0.0, 1.0);
}
