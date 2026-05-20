//
// Created by lance on 2022/10/31.
//

#ifndef MORROW_MATRIX3_H
#define MORROW_MATRIX3_H

#include <cstring>
#include <stdexcept>
#include <cmath>
#include <cstdint>
#include "Matrix4.h"

namespace morrow
{
namespace Math
{
class Matrix4;
class Matrix3
{
public:
    Matrix3();

    Matrix3(float n11, float n12, float n13, float n21, float n22, float n23, float n31, float n32, float n33);

    Matrix3(const Matrix3& matrix);

    float operator[](uint32_t index) const;

    Matrix3& set(float n11, float n12, float n13, float n21, float n22, float n23, float n31, float n32, float n33);

    Matrix3& identity();

    Matrix3& operator*=(const Matrix3& m);

    Matrix3& premultiply(const Matrix3& m);

    Matrix3& multiplyMatrices(const Matrix3& a, const Matrix3& b);

    Matrix3& operator*=(float scalar);

    float determinant();

    Matrix3 inverted() const;

    Matrix3& invert();

    Matrix3& transpose();

    void transposeIntoArray(float* r) const;

    Matrix3& setUvTransform(float tx, float ty, float sx, float sy, float rotation, float cx, float cy);

    Matrix3& scale(float sx, float sy);

    Matrix3& rotate(float theta);

    Matrix3& translate(float tx, float ty);

    bool operator==(const Matrix3& matrix);

    static Matrix3 fromArray(float* array, uint32_t offset = 0);

    void writeTo(float* array, uint32_t offset = 0);

    Matrix3& setFromMatrix4(const Matrix4& m);

public:
    static const float IDENTITY[];

    float elements[9];
};
}
}

#endif //MORROW_MATRIX3_H
