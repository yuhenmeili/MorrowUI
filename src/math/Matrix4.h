//
// Created by lance on 2022/10/23.
//

#ifndef MORROW_MATRIX4_H
#define MORROW_MATRIX4_H

#include "Vector3.h"
#include "Quaternion.h"
#include "Matrix3.h"

namespace morrow
{
namespace Math
{
class Vector3;

class Quaternion;

class Matrix3;

class Matrix4
{
public:
    Matrix4();

    Matrix4(
            float n11, float n12, float n13, float n14,
            float n21, float n22, float n23, float n24,
            float n31, float n32, float n33, float n34,
            float n41, float n42, float n43, float n44);

    Matrix4& lookAt(Vector3& eye, Vector3& target, Vector3& up);

    Matrix4& setToLookAt(Vector3& position, Vector3& target, Vector3& up);

    Matrix4& makeOrthographic(float left, float right, float top, float bottom, float near, float far);

    Matrix4& makePerspective(float left, float right, float top, float bottom, float near, float far);

    Matrix4& multiplyMatrices(const Matrix4& a, const Matrix4& b);

    Matrix4& operator*=(const Matrix4& m);

    Matrix4& multiply(const Matrix4& m);

    Matrix4& premultiply(const Matrix4& m);

    Matrix4& set(float n11, float n12, float n13, float n14, float n21, float n22, float n23, float n24, float n31,
                 float n32, float n33, float n34, float n41, float n42, float n43, float n44);

    Matrix4& copy(Matrix4& m);

    Matrix4& makeTranslation(float x, float y, float z);

    Matrix4& applyTranslation(Vector3& v);

    Matrix4& applyTranslation(float x, float y, float z);

    Vector3 getTranslation();

    Matrix4& identity();

    Matrix4& makeRotationX(float theta);

    Matrix4& makeRotationY(float theta);

    Matrix4& makeRotationZ(float theta);

    Matrix4& makeScale(float x, float y, float z);

    Matrix4& applyScale(float x, float y, float z);

    Vector3 getScale();

    Matrix4& makeRotationAxis(Vector3& axis, float angle);

    Matrix4& makeRotationFromQuaternion(Quaternion& quaternion);

    Matrix4& applyRotation(Quaternion& quaternion);

    Quaternion getQuaternion();

    void decompose(Vector3& position, Quaternion& quaternion, Vector3& scale) const;

    Matrix4& compose(Vector3& position, Quaternion& quaternion, Vector3& scale);

    Matrix4 inverted() const;

    bool operator==(const Matrix4& matrix) const;

    float operator[](uint32_t index) const;

//    float& operator[](uint32_t index);

    Matrix4& setFromMatrix3(const Matrix3& m);

    void log();

    Matrix4& transpose();

public:
    float elements[16];
};

inline Matrix4 operator*(const Matrix4& m1, const Matrix4& m2)
{
    return Matrix4().multiplyMatrices(m1, m2);
}
}
}

#endif //MORROW_MATRIX4_H
