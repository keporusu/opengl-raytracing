#include <vector>

class ModelLoader
{

public:
    static std::vector<float> GetTriangleVetices() { return TriangleVertices; }
    static std::vector<float> GetQuadVertices() { return QuadVertices; }
    static std::vector<unsigned int> GetQuadIndices() { return QuadIndices; }

private:
    inline static std::vector<float> TriangleVertices = {
        -0.5f, -0.5f, 0.0f,
        0.5f, -0.5f, 0.0f,
        0.0f, 0.5f, 0.0f};

    inline static std::vector<float> QuadVertices = {
        //座標3, uv座標2
        1.0f, 1.0f, 0.0f, 1.0f, 1.0f,   // top right
        1.0f, -1.0f, 0.0f, 1.0f, 0.0f,  // bottom right
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, // bottom left
        -1.0f, 1.0f, 0.0f, 0.0f, 1.0f   // top left
    };

    inline static std::vector<unsigned int> QuadIndices = {
        0, 1, 3, // first triangle
        1, 2, 3  // second triangle
    };
};
