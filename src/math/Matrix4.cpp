//
// Created by lance on 2022/10/23.
//

#include <cstdio>
#include "Matrix4.h"
#include "Log.h"

namespace morrow
{
namespace Math
{
Matrix4::Matrix4() : elements{
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
}
{
}

Matrix4::Matrix4(float n11, float n12, float n13, float n14, float n21, float n22, float n23, float n24, float n31,
                 float n32, float n33, float n34, float n41, float n42, float n43, float n44)
{

    set(n11, n12, n13, n14, n21, n22, n23, n24, n31,
        n32, n33, n34, n41, n42, n43, n44);
}

Matrix4& Matrix4::set(float n11, float n12, float n13, float n14, float n21, float n22, float n23, float n24, float n31,
                      float n32, float n33, float n34, float n41, float n42, float n43, float n44)
{
    float* te = elements;

    te[0] = n11;
    te[4] = n12;
    te[8] = n13;
    te[12] = n14;
    te[1] = n21;
    te[5] = n22;
    te[9] = n23;
    te[13] = n24;
    te[2] = n31;
    te[6] = n32;
    te[10] = n33;
    te[14] = n34;
    te[3] = n41;
    te[7] = n42;
    te[11] = n43;
    te[15] = n44;

    return *this;
}

Matrix4& Matrix4::copy(Matrix4& m)
{
    float* te = elements;
    float* me = m.elements;

    te[0] = me[0];
    te[1] = me[1];
    te[2] = me[2];
    te[3] = me[3];
    te[4] = me[4];
    te[5] = me[5];
    te[6] = me[6];
    te[7] = me[7];
    te[8] = me[8];
    te[9] = me[9];
    te[10] = me[10];
    te[11] = me[11];
    te[12] = me[12];
    te[13] = me[13];
    te[14] = me[14];
    te[15] = me[15];

    return *this;
}

Matrix4& Matrix4::lookAt(Vector3& eye, Vector3& target, Vector3& up)
{
    Vector3 _x;
    Vector3 _y;
    Vector3 _z;

    _z.subVectors(eye, target);

    if (_z.lengthSq() == 0.0f) {
        _z.z = 1.0f;
    }

    _z.normalize();
    _x.crossVectors(up, _z);

    if (_x.lengthSq() == 0.0f) {
        if (std::abs(up.z) == 1.0f) {
            _z.x += 0.0001f;
        } else {
            _z.z += 0.0001f;
        }
        _z.normalize();
        _x.crossVectors(up, _z);
    }

    _x.normalize();
    _y.crossVectors(_z, _x);

    elements[0] = _x.x;
    elements[4] = _y.x;
    elements[8] = _z.x;
    elements[1] = _x.y;
    elements[5] = _y.y;
    elements[9] = _z.y;
    elements[2] = _x.z;
    elements[6] = _y.z;
    elements[10] = _z.z;

    return *this;

}

Matrix4& Matrix4::setToLookAt(Vector3& position, Vector3& target, Vector3& up)
{
    identity();
    Vector3 tmpVec, l_vez, l_vex, l_vey;
    tmpVec.copy(target).sub(position);

    l_vez.copy(tmpVec).normalize();
    l_vex.copy(tmpVec).cross(up).normalize();
    l_vey.copy(l_vex).cross(l_vez).normalize();

    elements[0] = l_vex.x;
    elements[4] = l_vex.y;
    elements[8] = l_vex.z;
    elements[1] = l_vey.x;
    elements[5] = l_vey.y;
    elements[9] = l_vey.z;
    elements[2] = -l_vez.x;
    elements[6] = -l_vez.y;
    elements[10] = -l_vez.z;

    Matrix4 tmpMat;
    tmpMat.makeTranslation(-position.x, -position.y, -position.z);
    multiply(tmpMat);
    return *this;
}

Matrix4& Matrix4::makeOrthographic(float left, float right, float top, float bottom, float near, float far)
{
//    printf("left %f, %f, %f, %f, %f, %f",left,right,top,bottom,near,far);
    float w = 1.0f / (right - left);
    float h = 1.0f / (top - bottom);
    float p = 1.0f / (far - near);

    float x = (right + left) * w;
    float y = (top + bottom) * h;
    float z = (far + near) * p;
//    printf("w %f", w);

    elements[0] = 2.0f * w;
    elements[4] = 0.0f;
    elements[8] = 0.0f;
    elements[12] = -x;
    elements[1] = 0.0f;
    elements[5] = 2.0f * h;
    elements[9] = 0.0f;
    elements[13] = -y;
    elements[2] = 0.0f;
    elements[6] = 0.0f;
    elements[10] = -2.0f * p;
    elements[14] = -z;
    elements[3] = 0.0f;
    elements[7] = 0.0f;
    elements[11] = 0.0f;
    elements[15] = 1.0f;

    return *this;
}

Matrix4& Matrix4::makePerspective(float left, float right, float top, float bottom, float near, float far)
{
    float x = 2 * near / (right - left);
    float y = 2 * near / (top - bottom);

    float a = (right + left) / (right - left);
    float b = (top + bottom) / (top - bottom);
    float c = -(far + near) / (far - near);
    float d = -2 * far * near / (far - near);

    elements[0] = x;
    elements[4] = 0;
    elements[8] = a;
    elements[12] = 0;
    elements[1] = 0;
    elements[5] = y;
    elements[9] = b;
    elements[13] = 0;
    elements[2] = 0;
    elements[6] = 0;
    elements[10] = c;
    elements[14] = d;
    elements[3] = 0;
    elements[7] = 0;
    elements[11] = -1;
    elements[15] = 0;

    return *this;
}

Matrix4& Matrix4::multiplyMatrices(const Matrix4& a, const Matrix4& b)
{
    const float* ae = a.elements;
    const float* be = b.elements;
    float* te = elements;

    float a11 = ae[0], a12 = ae[4], a13 = ae[8], a14 = ae[12];
    float a21 = ae[1], a22 = ae[5], a23 = ae[9], a24 = ae[13];
    float a31 = ae[2], a32 = ae[6], a33 = ae[10], a34 = ae[14];
    float a41 = ae[3], a42 = ae[7], a43 = ae[11], a44 = ae[15];

    float b11 = be[0], b12 = be[4], b13 = be[8], b14 = be[12];
    float b21 = be[1], b22 = be[5], b23 = be[9], b24 = be[13];
    float b31 = be[2], b32 = be[6], b33 = be[10], b34 = be[14];
    float b41 = be[3], b42 = be[7], b43 = be[11], b44 = be[15];

    te[0] = a11 * b11 + a12 * b21 + a13 * b31 + a14 * b41;
    te[4] = a11 * b12 + a12 * b22 + a13 * b32 + a14 * b42;
    te[8] = a11 * b13 + a12 * b23 + a13 * b33 + a14 * b43;
    te[12] = a11 * b14 + a12 * b24 + a13 * b34 + a14 * b44;

    te[1] = a21 * b11 + a22 * b21 + a23 * b31 + a24 * b41;
    te[5] = a21 * b12 + a22 * b22 + a23 * b32 + a24 * b42;
    te[9] = a21 * b13 + a22 * b23 + a23 * b33 + a24 * b43;
    te[13] = a21 * b14 + a22 * b24 + a23 * b34 + a24 * b44;

    te[2] = a31 * b11 + a32 * b21 + a33 * b31 + a34 * b41;
    te[6] = a31 * b12 + a32 * b22 + a33 * b32 + a34 * b42;
    te[10] = a31 * b13 + a32 * b23 + a33 * b33 + a34 * b43;
    te[14] = a31 * b14 + a32 * b24 + a33 * b34 + a34 * b44;

    te[3] = a41 * b11 + a42 * b21 + a43 * b31 + a44 * b41;
    te[7] = a41 * b12 + a42 * b22 + a43 * b32 + a44 * b42;
    te[11] = a41 * b13 + a42 * b23 + a43 * b33 + a44 * b43;
    te[15] = a41 * b14 + a42 * b24 + a43 * b34 + a44 * b44;

    return *this;

}

Matrix4& Matrix4::multiply(const Matrix4& m)
{
    return multiplyMatrices(*this, m);
}

Matrix4& Matrix4::premultiply(const Matrix4& m)
{
    return multiplyMatrices(m, *this);
}

Matrix4& Matrix4::makeTranslation(float x, float y, float z)
{
    set(
            1.0f, 0.0f, 0.0f, x,
            0.0f, 1.0f, 0.0f, y,
            0.0f, 0.0f, 1.0f, z,
            0.0f, 0.0f, 0.0f, 1.0f
    );
    return *this;
}

Matrix4& Matrix4::applyTranslation(Vector3& v) {
    return applyTranslation(v.x, v.y, v.z);
}

Matrix4& Matrix4::applyTranslation(float x, float y, float z) {
    Matrix4 translationMatrix;
    translationMatrix.makeTranslation(x, y, z);
    return multiply(translationMatrix);
}

Vector3 Matrix4::getTranslation()
{
    return Vector3(elements[12], elements[13], elements[14]);
}

Matrix4& Matrix4::identity()
{
    set(
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
    );
    return *this;

}

Matrix4& Matrix4::makeRotationX(float theta)
{
    float c = std::cos(theta), s = std::sin(theta);
    set(
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, c, -s, 0.0f,
            0.0f, s, c, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
    );
    return *this;
}

Matrix4& Matrix4::makeRotationY(float theta)
{
    float c = std::cos(theta), s = std::sin(theta);
    set(
            c, 0.0f, s, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            -s, 0.0f, c, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
    );
    return *this;
}

Matrix4& Matrix4::makeRotationZ(float theta)
{
    float c = std::cos(theta), s = std::sin(theta);
    set(
            c, -s, 0.0f, 0.0f,
            s, c, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
    );
    return *this;
}

Matrix4& Matrix4::makeScale(float x, float y, float z)
{
    set(
            x, 0.0f, 0.0f, 0.0f,
            0.0f, y, 0.0f, 0.0f,
            0.0f, 0.0f, z, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
    );
    return *this;
}

Matrix4& Matrix4::applyScale(float x, float y, float z) {
    Matrix4 scaleMatrix;
    scaleMatrix.makeScale(x, y, z);
    return multiply(scaleMatrix);
}

Vector3 Matrix4::getScale() {
    Vector3 position;
    Quaternion rotation;
    Vector3 scale;
    decompose(position, rotation, scale);
    return scale;
}

Quaternion Matrix4::getQuaternion() {
    Vector3 position;
    Quaternion rotation;
    Vector3 scale;
    decompose(position, rotation, scale);
    return rotation;
}

void Matrix4::decompose(Vector3& position, Quaternion& quaternion, Vector3& scale) const {
    const float* te = elements;

    position.set(te[12], te[13], te[14]);

    float sx = std::sqrt(te[0] * te[0] + te[1] * te[1] + te[2] * te[2]);
    float sy = std::sqrt(te[4] * te[4] + te[5] * te[5] + te[6] * te[6]);
    float sz = std::sqrt(te[8] * te[8] + te[9] * te[9] + te[10] * te[10]);

    float det = te[0] * (te[5] * te[10] - te[6] * te[9])
              - te[4] * (te[1] * te[10] - te[2] * te[9])
              + te[8] * (te[1] * te[6] - te[2] * te[5]);
    if (det < 0.0f) {
        sx = -sx;
    }

    scale.set(sx, sy, sz);

    if (sx == 0.0f || sy == 0.0f || sz == 0.0f) {
        quaternion.set(0.0f, 0.0f, 0.0f, 1.0f);
        return;
    }

    Matrix4 rotationMatrix = *this;
    float* re = rotationMatrix.elements;

    float invSX = 1.0f / sx;
    float invSY = 1.0f / sy;
    float invSZ = 1.0f / sz;

    re[0] *= invSX; re[1] *= invSX; re[2] *= invSX;
    re[4] *= invSY; re[5] *= invSY; re[6] *= invSY;
    re[8] *= invSZ; re[9] *= invSZ; re[10] *= invSZ;

    quaternion.set(rotationMatrix).normalize();
}

Matrix4& Matrix4::makeRotationAxis(Vector3& axis, float angle)
{
    float c = std::cos(angle);
    float s = std::sin(angle);
    float t = 1.0f - c;
    float x = axis.x, y = axis.y, z = axis.z;
    float tx = t * x, ty = t * y;
    set(
            tx * x + c, tx * y - s * z, tx * z + s * y, 0.0f,
            tx * y + s * z, ty * y + c, ty * z - s * x, 0.0f,
            tx * z - s * y, ty * z + s * x, t * z * z + c, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
    );
    return *this;

}

Matrix4& Matrix4::makeRotationFromQuaternion(Quaternion& quaternion) {
    float x = quaternion.x, y = quaternion.y, z = quaternion.z, w = quaternion.w;
    float x2 = x + x, y2 = y + y, z2 = z + z;
    float xx = x * x2, xy = x * y2, xz = x * z2;
    float yy = y * y2, yz = y * z2, zz = z * z2;
    float wx = w * x2, wy = w * y2, wz = w * z2;

    set(
        1 - (yy + zz), xy - wz, xz + wy, 0.0f,
        xy + wz, 1 - (xx + zz), yz - wx, 0.0f,
        xz - wy, yz + wx, 1 - (xx + yy), 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );

    return *this;
}

Matrix4& Matrix4::applyRotation(Quaternion& quaternion) {
    Matrix4 rotationMatrix;
    rotationMatrix.makeRotationFromQuaternion(quaternion);
    return multiply(rotationMatrix);
}

Matrix4& Matrix4::compose(Vector3& position, Quaternion& quaternion, Vector3& scale)
{
    float* te = elements;

    float x = quaternion.x, y = quaternion.y, z = quaternion.z, w = quaternion.w;
    float x2 = x + x, y2 = y + y, z2 = z + z;
    float xx = x * x2, xy = x * y2, xz = x * z2;
    float yy = y * y2, yz = y * z2, zz = z * z2;
    float wx = w * x2, wy = w * y2, wz = w * z2;

    float sx = scale.x, sy = scale.y, sz = scale.z;

    te[0] = (1 - (yy + zz)) * sx;
    te[1] = (xy + wz) * sx;
    te[2] = (xz - wy) * sx;
    te[3] = 0;

    te[4] = (xy - wz) * sy;
    te[5] = (1 - (xx + zz)) * sy;
    te[6] = (yz + wx) * sy;
    te[7] = 0;

    te[8] = (xz + wy) * sz;
    te[9] = (yz - wx) * sz;
    te[10] = (1 - (xx + yy)) * sz;
    te[11] = 0;

    te[12] = position.x;
    te[13] = position.y;
    te[14] = position.z;
    te[15] = 1;

    return *this;
}

Matrix4 Matrix4::inverted() const
{
    // based on http://www.euclideanspace.com/maths/algebra/matrix/functions/inverse/fourD/index.htm
    Matrix4 inv;

    float* te = inv.elements;
    const float* me = elements,

            n11 = me[0], n21 = me[1], n31 = me[2], n41 = me[3],
            n12 = me[4], n22 = me[5], n32 = me[6], n42 = me[7],
            n13 = me[8], n23 = me[9], n33 = me[10], n43 = me[11],
            n14 = me[12], n24 = me[13], n34 = me[14], n44 = me[15],

            t11 = n23 * n34 * n42 - n24 * n33 * n42 + n24 * n32 * n43 - n22 * n34 * n43 - n23 * n32 * n44 + n22 * n33 * n44,
            t12 = n14 * n33 * n42 - n13 * n34 * n42 - n14 * n32 * n43 + n12 * n34 * n43 + n13 * n32 * n44 - n12 * n33 * n44,
            t13 = n13 * n24 * n42 - n14 * n23 * n42 + n14 * n22 * n43 - n12 * n24 * n43 - n13 * n22 * n44 + n12 * n23 * n44,
            t14 = n14 * n23 * n32 - n13 * n24 * n32 - n14 * n22 * n33 + n12 * n24 * n33 + n13 * n22 * n34 - n12 * n23 * n34;

    float det = n11 * t11 + n21 * t12 + n31 * t13 + n41 * t14;

    if (det == 0) {
        throw std::invalid_argument("Matrix4: cannnot invert, determinant is 0");
    }

    float detInv = 1.0f / det;

    te[0] = t11 * detInv;
    te[1] = (n24 * n33 * n41 - n23 * n34 * n41 - n24 * n31 * n43 + n21 * n34 * n43 + n23 * n31 * n44 - n21 * n33 * n44) * detInv;
    te[2] = (n22 * n34 * n41 - n24 * n32 * n41 + n24 * n31 * n42 - n21 * n34 * n42 - n22 * n31 * n44 + n21 * n32 * n44) * detInv;
    te[3] = (n23 * n32 * n41 - n22 * n33 * n41 - n23 * n31 * n42 + n21 * n33 * n42 + n22 * n31 * n43 - n21 * n32 * n43) * detInv;

    te[4] = t12 * detInv;
    te[5] = (n13 * n34 * n41 - n14 * n33 * n41 + n14 * n31 * n43 - n11 * n34 * n43 - n13 * n31 * n44 + n11 * n33 * n44) * detInv;
    te[6] = (n14 * n32 * n41 - n12 * n34 * n41 - n14 * n31 * n42 + n11 * n34 * n42 + n12 * n31 * n44 - n11 * n32 * n44) * detInv;
    te[7] = (n12 * n33 * n41 - n13 * n32 * n41 + n13 * n31 * n42 - n11 * n33 * n42 - n12 * n31 * n43 + n11 * n32 * n43) * detInv;

    te[8] = t13 * detInv;
    te[9] = (n14 * n23 * n41 - n13 * n24 * n41 - n14 * n21 * n43 + n11 * n24 * n43 + n13 * n21 * n44 - n11 * n23 * n44) * detInv;
    te[10] = (n12 * n24 * n41 - n14 * n22 * n41 + n14 * n21 * n42 - n11 * n24 * n42 - n12 * n21 * n44 + n11 * n22 * n44) * detInv;
    te[11] = (n13 * n22 * n41 - n12 * n23 * n41 - n13 * n21 * n42 + n11 * n23 * n42 + n12 * n21 * n43 - n11 * n22 * n43) * detInv;

    te[12] = t14 * detInv;
    te[13] = (n13 * n24 * n31 - n14 * n23 * n31 + n14 * n21 * n33 - n11 * n24 * n33 - n13 * n21 * n34 + n11 * n23 * n34) * detInv;
    te[14] = (n14 * n22 * n31 - n12 * n24 * n31 - n14 * n21 * n32 + n11 * n24 * n32 + n12 * n21 * n34 - n11 * n22 * n34) * detInv;
    te[15] = (n12 * n23 * n31 - n13 * n22 * n31 + n13 * n21 * n32 - n11 * n23 * n32 - n12 * n21 * n33 + n11 * n22 * n33) * detInv;

    return inv;
}

Matrix4& Matrix4::operator*=(const Matrix4& m)
{
    return multiplyMatrices(*this, m);
}

bool Matrix4::operator==(const Matrix4& matrix) const
{
//    const float* te = elements;
//    const float* me = matrix.elements;

    return std::memcmp(elements, matrix.elements, sizeof(elements)) == 0;
}

float Matrix4::operator[](uint32_t index) const
{
    return elements[index];
}

//float& Matrix4::operator[](uint32_t index)
//{
//    return elements[index];
//}

Matrix4& Matrix4::setFromMatrix3(const Matrix3& m)
{
    const float* me = m.elements;
    set(
            me[0], me[3], me[6], 0,
            me[1], me[4], me[7], 0,
            me[2], me[5], me[8], 0,
            0, 0, 0, 1
    );
    return *this;
}

void Matrix4::log()
{
    LOG_I("{},{},{},{}; {},{},{},{}; {},{},{},{}; {},{},{},{}", elements[0], elements[1], elements[2], elements[3],
           elements[4], elements[5], elements[6], elements[7],
           elements[8], elements[9], elements[10], elements[11],
           elements[12], elements[13], elements[14], elements[15]);
}

Matrix4& Matrix4::transpose()
{
    float* te = elements;
    float tmp;

    tmp = te[1];
    te[1] = te[4];
    te[4] = tmp;
    tmp = te[2];
    te[2] = te[8];
    te[8] = tmp;
    tmp = te[6];
    te[6] = te[9];
    te[9] = tmp;
    tmp = te[3];
    te[3] = te[12];
    te[12] = tmp;
    tmp = te[7];
    te[7] = te[13];
    te[13] = tmp;
    tmp = te[11];
    te[11] = te[14];
    te[14] = tmp;

    return *this;
}

}
}
