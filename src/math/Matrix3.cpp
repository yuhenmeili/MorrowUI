//
// Created by lance on 2022/10/31.
//

#include "Matrix3.h"
namespace morrow
{
namespace Math
{
const float Matrix3::IDENTITY[] = {
        1, 0, 0,
        0, 1, 0,
        0, 0, 1
};

Matrix3::Matrix3()
{
    identity();
}

Matrix3::Matrix3(float n11, float n12, float n13, float n21, float n22, float n23, float n31, float n32, float n33)
{
    set(n11, n12, n13, n21, n22, n23, n31, n32, n33);
}

Matrix3::Matrix3(const Matrix3& matrix)
{
    std::memcpy(elements, matrix.elements, sizeof(elements));
}

float Matrix3::operator[](uint32_t index) const
{ return elements[index]; }

Matrix3& Matrix3::set(float n11, float n12, float n13, float n21, float n22, float n23, float n31, float n32, float n33)
{
    float* te = elements;

    te[0] = n11;
    te[1] = n21;
    te[2] = n31;
    te[3] = n12;
    te[4] = n22;
    te[5] = n32;
    te[6] = n13;
    te[7] = n23;
    te[8] = n33;

    return *this;
}

Matrix3& Matrix3::identity()
{
    std::memcpy(elements, IDENTITY, sizeof(elements));

    return *this;
}

Matrix3& Matrix3::operator*=(const Matrix3& m)
{
    return multiplyMatrices(*this, m);
}

Matrix3& Matrix3::premultiply(const Matrix3& m)
{
    return multiplyMatrices(m, *this);
}

Matrix3& Matrix3::multiplyMatrices(const Matrix3& a, const Matrix3& b)
{
    const float* ae = a.elements;
    const float* be = b.elements;
    float* te = elements;

    float a11 = ae[0], a12 = ae[3], a13 = ae[6];
    float a21 = ae[1], a22 = ae[4], a23 = ae[7];
    float a31 = ae[2], a32 = ae[5], a33 = ae[8];

    float b11 = be[0], b12 = be[3], b13 = be[6];
    float b21 = be[1], b22 = be[4], b23 = be[7];
    float b31 = be[2], b32 = be[5], b33 = be[8];

    te[0] = a11 * b11 + a12 * b21 + a13 * b31;
    te[3] = a11 * b12 + a12 * b22 + a13 * b32;
    te[6] = a11 * b13 + a12 * b23 + a13 * b33;

    te[1] = a21 * b11 + a22 * b21 + a23 * b31;
    te[4] = a21 * b12 + a22 * b22 + a23 * b32;
    te[7] = a21 * b13 + a22 * b23 + a23 * b33;

    te[2] = a31 * b11 + a32 * b21 + a33 * b31;
    te[5] = a31 * b12 + a32 * b22 + a33 * b32;
    te[8] = a31 * b13 + a32 * b23 + a33 * b33;

    return *this;
}

Matrix3& Matrix3::operator*=(float scalar)
{
    float* te = elements;

    te[0] *= scalar;
    te[3] *= scalar;
    te[6] *= scalar;
    te[1] *= scalar;
    te[4] *= scalar;
    te[7] *= scalar;
    te[2] *= scalar;
    te[5] *= scalar;
    te[8] *= scalar;

    return *this;
}

float Matrix3::determinant()
{
    float* te = elements;

    float a = te[0], b = te[1], c = te[2],
            d = te[3], e = te[4], f = te[5],
            g = te[6], h = te[7], i = te[8];

    return a * e * i - a * f * h - b * d * i + b * f * g + c * d * h - c * e * g;
}

Matrix3 Matrix3::inverted() const
{
    return Matrix3(*this).invert();
}

Matrix3& Matrix3::invert()
{
    float* me = elements,

            n11 = me[0], n21 = me[1], n31 = me[2],
            n12 = me[3], n22 = me[4], n32 = me[5],
            n13 = me[6], n23 = me[7], n33 = me[8],

            t11 = n33 * n22 - n32 * n23,
            t12 = n32 * n13 - n33 * n12,
            t13 = n23 * n12 - n22 * n13,

            det = n11 * t11 + n21 * t12 + n31 * t13;

    if (det == 0) {
        throw std::invalid_argument("can't invert matrix, determinant is 0");
    }

    float detInv = 1.0f / det;

    me[0] = t11 * detInv;
    me[1] = (n31 * n23 - n33 * n21) * detInv;
    me[2] = (n32 * n21 - n31 * n22) * detInv;

    me[3] = t12 * detInv;
    me[4] = (n33 * n11 - n31 * n13) * detInv;
    me[5] = (n31 * n12 - n32 * n11) * detInv;

    me[6] = t13 * detInv;
    me[7] = (n21 * n13 - n23 * n11) * detInv;
    me[8] = (n22 * n11 - n21 * n12) * detInv;

    return *this;
}

Matrix3& Matrix3::transpose()
{
    float tmp, * m = elements;

    tmp = m[1];
    m[1] = m[3];
    m[3] = tmp;
    tmp = m[2];
    m[2] = m[6];
    m[6] = tmp;
    tmp = m[5];
    m[5] = m[7];
    m[7] = tmp;

    return *this;
}

void Matrix3::transposeIntoArray(float* r) const
{
    const float* m = elements;

    r[0] = m[0];
    r[1] = m[3];
    r[2] = m[6];
    r[3] = m[1];
    r[4] = m[4];
    r[5] = m[7];
    r[6] = m[2];
    r[7] = m[5];
    r[8] = m[8];
}

Matrix3& Matrix3::setUvTransform(float tx, float ty, float sx, float sy, float rotation, float cx, float cy)
{
    float c = std::cos(rotation);
    float s = std::sin(rotation);

    return set(
            sx * c, sx * s, -sx * (c * cx + s * cy) + cx + tx,
            -sy * s, sy * c, -sy * (-s * cx + c * cy) + cy + ty,
            0, 0, 1
    );
}

Matrix3& Matrix3::scale(float sx, float sy)
{
    elements[0] *= sx;
    elements[3] *= sx;
    elements[6] *= sx;
    elements[1] *= sy;
    elements[4] *= sy;
    elements[7] *= sy;

    return *this;
}

Matrix3& Matrix3::rotate(float theta)
{
    float c = std::cos(theta);
    float s = std::sin(theta);

    float a11 = elements[0], a12 = elements[3], a13 = elements[6];
    float a21 = elements[1], a22 = elements[4], a23 = elements[7];

    elements[0] = c * a11 + s * a21;
    elements[3] = c * a12 + s * a22;
    elements[6] = c * a13 + s * a23;

    elements[1] = -s * a11 + c * a21;
    elements[4] = -s * a12 + c * a22;
    elements[7] = -s * a13 + c * a23;

    return *this;
}

Matrix3& Matrix3::translate(float tx, float ty)
{
    float* te = elements;

    te[0] += tx * te[2];
    te[3] += tx * te[5];
    te[6] += tx * te[8];
    te[1] += ty * te[2];
    te[4] += ty * te[5];
    te[7] += ty * te[8];

    return *this;
}

bool Matrix3::operator==(const Matrix3& matrix)
{
    float* te = elements;
    const float* me = matrix.elements;

    for (unsigned i = 0; i < 9; i++) {
        if (te[i] != me[i]) return false;
    }

    return true;
}

Matrix3 Matrix3::fromArray(float* array, uint32_t offset)
{
    Matrix3 matrix;
    std::memcpy(matrix.elements, array + offset, sizeof(elements));
    return matrix;
}

void Matrix3::writeTo(float* array, uint32_t offset)
{
    float* te = elements;

    array[offset] = te[0];
    array[offset + 1] = te[1];
    array[offset + 2] = te[2];

    array[offset + 3] = te[3];
    array[offset + 4] = te[4];
    array[offset + 5] = te[5];

    array[offset + 6] = te[6];
    array[offset + 7] = te[7];
    array[offset + 8] = te[8];
}

Matrix3& Matrix3::setFromMatrix4(const Matrix4& m)
{
    const float* me = m.elements;
    set(
            me[ 0 ], me[ 4 ], me[ 8 ],
            me[ 1 ], me[ 5 ], me[ 9 ],
            me[ 2 ], me[ 6 ], me[ 10 ]
    );
    return *this;
}
}
}
