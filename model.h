#pragma once

int NumPosisionElements = 3;
float Positions[] = {
     3.0f,   5.20056f,   2.98357f,
    -3.0f,   5.20056f,   2.98357f,
    -3.0f,   0.004403f, -0.0164322f,
     3.0f,   5.20056f,   2.98357f,
    -3.0f,   0.004403f, -0.0164322f,
     3.0f,   0.004403f, -0.0164322f,
};

int NumTexcoordElements = 2;
float Texcoords[] = {
    1.0f, 0.0f,
    0.0f, 0.0f,
    0.0f, 1.0f,
    1.0f, 0.0f,
    0.0f, 1.0f,
    1.0f, 1.0f,
};

unsigned short Indices[] = {
    0, 1, 2,
    3, 4, 5,
};

const char* TexFilename = "assets\\plane\\girl.png";
