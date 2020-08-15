/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * Copyright 2016-2020 Mark Callow
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef VECMATH_9B7E1CFE346D11E6AFA2D7DC87495A69_H
#define VECMATH_9B7E1CFE346D11E6AFA2D7DC87495A69_H

/**
 * @file vecmath.h
 * @~English
 *
 * @brief Vector math package modelled after GLSL.
 */

#include <assert.h>
#include <math.h>

struct vec2 {
    // Anonymous unions are portable.
    union {
        float x;
        float r;
    };
    union {
        float y;
        float g;
    };

    vec2() { };
};

struct vec3 {
    union {
        float x;
        float r;
    };
    union {
        float y;
        float g;
    };
    union {
        float z;
        float b;
    };

    vec3() { }

    vec3(float x, float y, float z) : x(x), y(y), z(z) { }

    vec3(const vec3& value)
        : x(value.x), y(value.y), z(value.z) { }

    vec3 operator-() const
    {
        return vec3(-x, -y, -z);
    }

    vec3 operator-(const vec3& value) const
    {
        return vec3(this->x - value.x, this->y - value.y, this->z - value.z);
    }

    vec3 operator/(float divisor) const
    {
        return vec3(x / divisor, y / divisor, z / divisor);
    }

    vec3& operator/=(float divisor)
    {
        x /= divisor;
        y /= divisor;
        z /= divisor;
        return *this;
    }

    vec3& operator=(const vec3& value)
    {
        x = value.x;
        y = value.y;
        z = value.z;
        return *this;
    }

    float operator[](int i) const
    {
        switch (i) {
          case 0: return x;
          case 1: return y;
          case 2: return z;
          default:
            assert(false);
            return z;
        }
    }

    float& operator[](int i)
    {
        switch (i) {
          case 0: return x;
          case 1: return y;
          case 2: return z;
          default:
            assert(false);
            return z;
        }
    }

    vec3 cross(const vec3& value) const
    {
        return vec3::cross(*this, value);
    }

    float dot(const vec3& vec) const
    {
        return vec3::dot(*this, vec);
    }

    float length() const
    {
        return sqrt(dot(*this));
    }

    vec3& normalize()
    {
        float length = this->length();
        return (length > 0.0f ? *this /= length : *this);
    }

    static vec3 cross(const vec3& a, const vec3& b)
    {
        return vec3(a.y * b.z - a.z * b.y,
                    a.z * b.x - a.x * b.z,
                    a.x * b.y - a.y * b.x);
    }

    static float dot(const vec3& a, const vec3& b)
    {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    static vec3 normalize (const vec3& input)
    {
        float length = input.length();
        return (length > 0.0f ? input / length : input);
    }
};


struct vec4 {

    union {
        float x;
        float r;
    };
    union {
        float y;
        float g;
    };
    union {
        float z;
        float b;
    };
    union {
        float w;
        float a;
    };

    vec4() { }

    vec4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) { }

    vec4(const vec3& value, float w)
        : x(value.x), y(value.y), z(value.z), w(w) { }

    vec4(const vec4& value)
        : x(value.x), y(value.y), z(value.z), w(value.w) { }

    vec4 operator/(float divisor) const
    {
        return vec4(x / divisor, y / divisor, z / divisor, w / divisor);
    }

    vec4& operator/=(float divisor)
    {
        x /= divisor;
        y /= divisor;
        z /= divisor;
        w /= divisor;
        return *this;
    }

    vec4 operator*(float multiplicand) const
    {
        return vec4(x * multiplicand, y * multiplicand, z * multiplicand,
                    w * multiplicand);
    }

    vec4& operator=(const vec4& value)
    {
        x = value.x;
        y = value.y;
        z = value.z;
        w = value.w;
        return *this;
    }

    float operator[](int i) const
    {
        switch (i) {
          case 0: return x;
          case 1: return y;
          case 2: return z;
          case 3: return w;
          default:
            assert(false);
            return z;
        }
    }

    float& operator[](int i)
    {
        switch (i) {
          case 0: return x;
          case 1: return y;
          case 2: return z;
          case 3: return w;
          default:
            assert(false);
            return z;
        }
    }

    float dot(const vec4& vec) const
    {
        return vec4::dot(*this, vec);
    }

    float length() const
    {
        return sqrt(dot(*this));
    }

    static float dot(const vec4& a, const vec4& b)
    {
        return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
    }

    static vec4 normalize (const vec4& input)
    {
        float length = input.length();
        return (length > 0.0f ? input / length : input);
    }
};

struct mat3 {
    vec3 m[3];

    mat3()
    {
        m[0] = vec3(1.0f, 0.0f, 0.0f);
        m[1] = vec3(0.0f, 1.0f, 0.0f);
        m[2] = vec3(0.0f, 0.0f, 1.0f);
    }

    mat3(const vec3& value1, const vec3& value2, const vec3& value3)
    {
        m[0] = value1;
        m[1] = value2;
        m[2] = value3;
    }

    mat3 transpose() const
    {
        return mat3(vec3(m[0].x, m[1].x, m[2].x),
                    vec3(m[0].y, m[1].y, m[2].y),
                    vec3(m[0].z, m[1].z, m[2].z));
    }
};

struct mat4 {
    vec4 m[4];

    mat4()
    {
        m[0] = vec4(1.0f, 0.0f, 0.0f, 0.0f);
        m[1] = vec4(0.0f, 1.0f, 0.0f, 0.0f);
        m[2] = vec4(0.0f, 0.0f, 1.0f, 0.0f);
        m[3] = vec4(0.0f, 0.0f, 0.0f, 1.0f);
    }

    mat4(const vec4& value1, const vec4& value2,
         const vec4& value3, const vec4& value4)
    {
        m[0] = value1;
        m[1] = value2;
        m[2] = value3;
        m[3] = value4;
    }

    mat4 operator*(float value) const
    {
        return mat4(m[0] * value, m[1] * value, m[2] * value, m[3] * value);
    }

    mat4 operator*(const mat4& value) const
    {
        mat4 right = value.transpose();
        return mat4(
           vec4(m[0].dot(right.m[0]), m[0].dot(right.m[1]),
                m[0].dot(right.m[2]), m[0].dot(right.m[3])),
           vec4(m[1].dot(right.m[0]), m[1].dot(right.m[1]),
                m[1].dot(right.m[2]), m[1].dot(right.m[3])),
           vec4(m[2].dot(right.m[0]), m[2].dot(right.m[1]),
                m[2].dot(right.m[2]), m[2].dot(right.m[3])),
           vec4(m[3].dot(right.m[0]), m[3].dot(right.m[1]),
                m[3].dot(right.m[2]), m[3].dot(right.m[3]))
       );
    }

    vec4 operator*(const vec4& value) const
    {
        return vec4(m[0].dot(value), m[1].dot(value),
                    m[2].dot(value), m[3].dot(value));
    }

    vec4& operator[](int i)
    {
        return m[i];
    }

    mat4 transpose() const
    {
        return mat4(vec4(m[0][0], m[1][0], m[2][0], m[3][0]),
                    vec4(m[0][1], m[1][1], m[2][1], m[3][1]),
                    vec4(m[0][2], m[1][2], m[2][2], m[3][2]),
                    vec4(m[0][3], m[1][3], m[2][3], m[3][3]));
    }

    static mat4 translate(const vec3& trans)
    {
        return mat4(vec4(1.0f, 0.0f, 0.0f, trans.x),
                    vec4(0.0f, 1.0f, 0.0f, trans.y),
                    vec4(0.0f, 0.0f, 1.0f, trans.z),
                    vec4(0.0f, 0.0f, 0.0f, 1.0f));
    }

    static mat4 translate(float x, float y, float z)
    {
        return mat4(vec4(1.0f, 0.0f, 0.0f, x),
                    vec4(0.0f, 1.0f, 0.0f, y),
                    vec4(0.0f, 0.0f, 1.0f, z),
                    vec4(0.0f, 0.0f, 0.0f, 1.0f));
    }

    static mat4 scale(const vec3& scale)
    {
        return mat4(vec4(scale.x, 0.0f, 0.0f, 0.0f),
                    vec4(0.0f, scale.y, 0.0f, 0.0f),
                    vec4(0.0f, 0.0f, scale.z, 0.0f),
                    vec4(0.0f, 0.0f, 0.0f, 1.0f));
    }

    static mat4 scale(float x, float y, float z)
    {
        return mat4(vec4(x, 0.0f, 0.0f, 0.0f),
                    vec4(0.0f, y, 0.0f, 0.0f),
                    vec4(0.0f, 0.0f, z, 0.0f),
                    vec4(0.0f, 0.0f, 0.0f, 1.0f));
    }

    static mat4 frustum(float left, float right, float bottom, float top,
                        float zNear, float zFar)
    {
        return mat4(
            vec4(2.0f * zNear / (right - left), 0.0f,
                 (right + left) / (right - left), 0.0f),
            vec4(0.0f, 2.0f * zNear / (top - bottom),
                 (top + bottom) / (top - bottom), 0.0f),
            vec4(0.0f, 0.0f, (zFar + zNear) / (zNear - zFar),
                 2.0f * zFar * zNear / (zNear - zFar)),
            vec4(0.0f, 0.0f, -1.0f, 0.0f)
        );
    }

    static mat4 ortho(float left, float right, float bottom, float top,
                      float zNear, float zFar)
    {
        return mat4(
             vec4(2.0f / (right - left), 0.0f, 0.0f,
                  (right + left) / (left - right)),
             vec4(0.0f, 2.0f / (top - bottom), 0.0f,
                  (top + bottom) / (bottom - top)),
             vec4(0.0f, 0.0f, 2.0f / (zNear - zFar),
                  (zFar + zNear) / (zNear - zFar)),
             vec4(0.0f, 0.0f, 0.0f, 1.0f)
        );
    }

    static mat4 lookAt(const vec3& eye, const vec3& center, const vec3& up)
    {
        vec3 const forward(vec3::normalize(center - eye));
        vec3 const side(vec3::normalize(vec3::cross(forward, up)));
        vec3 const u(vec3::cross(side, forward));

        mat4 result;
        result[0][0] = side.x;
        result[1][0] = side.y;
        result[2][0] = side.z;
        result[0][1] = u.x;
        result[1][1] = u.y;
        result[2][1] = u.z;
        result[0][2] =-forward.x;
        result[1][2] =-forward.y;
        result[2][2] =-forward.z;
        result[3][0] =-vec3::dot(side, eye);
        result[3][1] =-vec3::dot(u, eye);
        result[3][2] = vec3::dot(forward, eye);

        return result;
    }

    static mat4 lookAt(float eyeX, float eyeY, float eyeZ,
                       float centerX, float centerY, float centerZ,
                       float upX, float upY, float upZ)
    {
        return mat4::lookAt(vec3(eyeX, eyeY, eyeZ),
                            vec3(centerX, centerY, centerZ),
                            vec3(upX, upY, upZ));
    }

    static mat4 perspective(float fovY, float aspect, float zNear, float zFar)
    {
        float scaleY = 1.0f / tan(fovY * 3.1415962f / 360.0f);
        return mat4(
             vec4(scaleY / aspect, 0.0f, 0.0f, 0.0f),
             vec4(0.0f, scaleY, 0.0f, 0.0f),
             vec4(0.0f, 0.0f, (zFar + zNear) / (zNear - zFar),
                  (2.0f * zFar * zNear) / (zNear - zFar)),
             vec4(0.0f, 0.0f, -1.0f, 0.0f)
        );
    }
};

#endif /* VECMATH_9B7E1CFE-346D-11E6-AFA2-D7DC87495A69_H */
