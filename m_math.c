#include "m_math.h"
#include <stdlib.h>
#include <math.h>
#include <assert.h>


#define CROSS(dest,v1,v2) \
    dest[0]=v1[1]*v2[2]-v1[2]*v2[1]; \
    dest[1]=v1[2]*v2[0]-v1[0]*v2[2]; \
    dest[2]=v1[0]*v2[1]-v1[1]*v2[0];

#define DOT(v1,v2) (v1[0]*v2[0]+v1[1]*v2[1]+v1[2]*v2[2])

#define SUB(dest,v1,v2) \
    dest[0]=v1[0]-v2[0]; \
    dest[1]=v1[1]-v2[1]; \
    dest[2]=v1[2]-v2[2];

_Static_assert(sizeof(Vec3) == 12, "Bad size for Vec3. Should be exactly 3 floats wide.");

static void m_Mult_Mat2Vec2(const Mat2* m, Vec2* v)
{
    float x = m->x00 * v->x + m->x01 * v->y;
    v->y = m->x10 * v->x + m->x11 * v->y;
    v->x = x;
}

Mat4 m_Ident_Mat4(void)
{
    Mat4 m = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1,
    };
    return m;
};

Mat4 m_BuildPerspective(const float nearDist, const float farDist)
{
    /*
     * Formula: 
     * 1   0   0   0
     * 0   1   0   0
     * 0   0   A  -1
     * 0   0   B   0
     *
     * A = -f / (f - n)
     * B = - (f * n) / (f - n)
     *
     * f = distance to far clip plane from camera
     * n = distance to close clip plane from camera
     *
     * Note: n cannot be 0!
     * TODO: derive this for yourself.
     */
    const float f = farDist;
    const float n = nearDist;
    const float A = -1 * f / (f - n);
    const float B = -1 * f * n / (f - n);
    Mat4 m = {
        1,   0,  0,  0,
        0,  -1,  0,  0,
        0,   0,  A,  -1,
        0,   0,  B,  0
    };
    return m;
}

void m_Rotate_Vec2(const float angle, Vec2* v)
{
    Mat2 rot = {
        .x00 =  cos(angle),
        .x01 = -sin(angle),
        .x10 =  sin(angle),
        .x11 =  cos(angle)
    };
    m_Mult_Mat2Vec2(&rot, v);
}

Vec3 m_RotateY_Vec3(const float angle, const Vec3 *v)
{
    Mat4 m = m_Ident_Mat4();
    m = m_RotateY_Mat4(angle, &m);
    return m_Mult_Mat4Vec3(&m, v);
}

Vec3 m_Add_Vec3(const Vec3* a, const Vec3* b)
{
    return (Vec3){
        a->x[0] + b->x[0], 
        a->x[1] + b->x[1], 
        a->x[2] + b->x[2]};
}

Vec3 m_Sub_Vec3(const Vec3* a, const Vec3* b)
{
    return (Vec3){
        a->x[0] - b->x[0],
        a->x[1] - b->x[1],
        a->x[2] - b->x[2]};
}

float m_Dot(const Vec3* a, const Vec3* b)
{
    return a->x[0] * b->x[0] 
         + a->x[1] * b->x[1] 
         + a->x[2] * b->x[2];
}

Vec3 m_Cross(const Vec3 *a, const Vec3 *b)
{
    Vec3 v;
    v.x[0] = a->x[1] * b->x[2] - a->x[2] * b->x[1];
    v.x[1] = a->x[2] * b->x[0] - a->x[0] * b->x[2];
    v.x[2] = a->x[0] * b->x[1] - a->x[1] * b->x[0];
    return v;
}

Vec3 m_Mult_Mat4Vec3(const Mat4* m, const Vec3* v)
{
    Vec3 out;
    for (int i = 0; i < 3; i++) 
    {
        out.x[i] = 0.0;
        for (int j = 0; j < 3; j++) 
        {
            out.x[i] += m->x[j][i] * v->x[j];
        }
    }
    return out;
}

Vec4 m_Mult_Mat4Vec4(const Mat4* m, const Vec4* v)
{
    Vec4 out;
    for (int i = 0; i < 4; i++) 
    {
        out.x[i] = 0.0;
        for (int j = 0; j < 4; j++) 
        {
            out.x[i] += m->x[j][i] * v->x[j];
        }
    }
    return out;
}

Mat4 m_Mult_Mat4(const Mat4* a, const Mat4* b)
{
    Mat4 out;
    for (int i = 0; i < 4; i++) 
        for (int j = 0; j < 4; j++) 
        {
            out.x[i][j] = 0;
            for (int k = 0; k < 4; k++) 
                out.x[i][j] += a->x[i][k] * b->x[k][j];
        }
    return out;
}

Mat4 m_BuildRotate(const float angle, const Vec3* axis)
{
    const float x = axis->x[0];
    const float y = axis->x[1];
    const float z = axis->x[2];
    const float a = angle;
    const Vec3 col1 = {
        cos(a) + x*x*(1 - cos(a)),
        x*y*(1 - cos(a)) + z*sin(a),
        z*x*(1 - cos(a)) - y*sin(a)
    };
    const Vec3 col2 = {
        x*y*(1 - cos(a)) - z*sin(a),
        cos(a) + y*y*(1 - cos(a)),
        z*y*(1 - cos(a)) + x*sin(a)
    };
    const Vec3 col3 = {
        x*z*(1 - cos(a)) + y*sin(a),
        y*z*(1 - cos(a)) - x*sin(a),
        cos(a) + z*z*(1 - cos(a))
    };
    return m_BuildFromBasis_Mat4(col1.x, col2.x, col3.x);
}

Mat4 m_RotateY_Mat4(const float angle, const Mat4* m)
{
    const float c = cos(angle);
    const float s = sin(angle);
    const Mat4 rot = {
        c, 0,-s, 0,
        0, 1, 0, 0,
        s, 0, c, 0,
        0, 0, 0, 1,
    };
    return m_Mult_Mat4(&rot, m);
}

Mat4 m_RotateZ_Mat4(const float angle, const Mat4* m)
{
    const float c = cos(angle);
    const float s = sin(angle);
    const Mat4 rot = {
        c, s, 0, 0,
       -s, c, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1,
    };
    return m_Mult_Mat4(&rot, m);
}

Mat4 m_Translate_Mat4(const Vec3 t, const Mat4 *m)
{
    const Mat4 trans = {
        1,     0,   0, 0,
        0,     1,   0, 0,
        0,     0,   1, 0,
        t.x[0], t.x[1], t.x[2], 1
    };
    return m_Mult_Mat4(&trans, m);
}

Mat4 m_BuildFromBasis_Mat4(const float x[3], const float y[3], const float z[3])
{
    const Mat4 m = (Mat4){
        x[0], x[1], x[2], 0,
        y[0], y[1], y[2], 0,
        z[0], z[1], z[2], 0,
        0,       0,    0, 1
    };
    return m;
}

Mat4 m_LookAt(const Vec3* pos, const Vec3* target, const Vec3* up)
{
    Vec3 temp = m_Sub_Vec3(target, pos);
    const Vec3 dir = m_Normalize_Vec3(&temp);
    temp = m_Cross(&dir, up);
    const Vec3 x = m_Normalize_Vec3(&temp);
    temp = m_Cross(&x, &dir);
    const Vec3 y = m_Normalize_Vec3(&temp);
    const Vec3 z = m_Scale_Vec3(-1, &dir);
    Mat4 m = m_BuildFromBasis_Mat4(x.x, y.x, z.x);
    m = m_Transpose_Mat4(&m);
    m = m_Translate_Mat4(m_Scale_Vec3(-1, pos), &m);
    return m;
}

void m_ScaleUniform_Mat4(const float s, Mat4 *m)
{
    for (int i = 0; i < 3; i++) 
    {
        for (int j = 0; j < 3; j++) 
        {
            m->x[i][j] *= s;
        }
    }
}

void m_ScaleNonUniform_Mat4(const Vec3 s, Mat4 *m)
{
    for (int i = 0; i < 3; i++) 
    {
        m->x[i][i] = s.x[i];
    }
}

Mat4 m_Transpose_Mat4(const Mat4 *m)
{
    Mat4 out;
    for (int i = 0; i < 4; i++) 
    {
        for (int j = 0; j < 4; j++) 
        {
            out.x[i][j] = m->x[j][i];
        }  
    }
    return out;
}

void m_Translate(const Vec2 t, Vec2* v)
{
    v->x += t.x;
    v->y += t.y;
}

void m_Scale(const float s, Vec2* v)
{
    v->x *= s;
    v->y *= s;
}

float m_Length(const Vec2 v)
{
    return sqrt(m_Length2(v));
}

float m_Length2(const Vec2 v)
{
    return v.x * v.x + v.y * v.y;
}

void m_Add(const Vec2 v1, Vec2* v2)
{
    v2->x += v1.x;
    v2->y += v1.y;
}

Vec2 m_Subtract(const Vec2 v1, const Vec2 v2)
{
    Vec2 v;
    v.x = v1.x - v2.x;
    v.y = v1.y - v2.y;
    return v;
}

float m_Rand(void)
{
    return rand() / (float)RAND_MAX;
}

float m_RandNeg(void)
{
    float r = rand() / (float)RAND_MAX;
    return r * 2 - 1;
}

float m_Det2x2(const Mat2 m)
{
    return m.x00 * m.x11 - m.x10 * m.x01;
}

Vec2 m_PolarToCart(const float angle, const float radius)
{
    Vec2 v = {
        cos(angle) * radius,
        sin(angle) * radius
    };
    return v;
}

Vec3 m_GetTranslation_Mat4(const Mat4* m)
{
    return (Vec3){m->x[3][0], m->x[3][1], m->x[3][2]};
}

Vec3 m_GetLocalZ_Mat4(const Mat4 *m)
{
    return (Vec3){
        m->x[2][0]/* + m->x[3][0] */, 
        m->x[2][1]/* + m->x[3][1] */, 
        m->x[2][2]/* + m->x[3][2] */};
}

float m_Length_Vec3(const Vec3* v)
{
    return sqrt(v->x[0] * v->x[0] + v->x[1] * v->x[1] + v->x[2] * v->x[2]);
}

float m_Length_Vec4(const Vec4* v)
{
    return sqrt(v->x[0] * v->x[0] + v->x[1] * v->x[1] + v->x[2] * v->x[2] + v->x[3] * v->x[3]);
}

Vec3 m_Scale_Vec3(const float s, const Vec3* v)
{
    return (Vec3){v->x[0] * s, v->x[1] * s, v->x[2] * s};
}

Vec3 m_Normalize_Vec3(const Vec3* v)
{
    const float m = m_Length_Vec3(v);
    assert(m != 0.0);
    return (Vec3){v->x[0] / m, v->x[1] / m, v->x[2] / m};
}

Vec4 m_Normalize_Vec4(const Vec4* v)
{
    const float m = m_Length_Vec4(v);
    assert(m != 0.0);
    return (Vec4){v->x[0] / m, v->x[1] / m, v->x[2] / m, v->x[3] / m};
}

Mat4 m_Invert4x4(const Mat4* const mat)
{
    const float* const m = (float*)mat->x;
    float inv[16];

    inv[0] = m[5]  * m[10] * m[15] - 
             m[5]  * m[11] * m[14] - 
             m[9]  * m[6]  * m[15] + 
             m[9]  * m[7]  * m[14] +
             m[13] * m[6]  * m[11] - 
             m[13] * m[7]  * m[10];

    inv[4] = -m[4]  * m[10] * m[15] + 
              m[4]  * m[11] * m[14] + 
              m[8]  * m[6]  * m[15] - 
              m[8]  * m[7]  * m[14] - 
              m[12] * m[6]  * m[11] + 
              m[12] * m[7]  * m[10];

    inv[8] = m[4]  * m[9] * m[15] - 
             m[4]  * m[11] * m[13] - 
             m[8]  * m[5] * m[15] + 
             m[8]  * m[7] * m[13] + 
             m[12] * m[5] * m[11] - 
             m[12] * m[7] * m[9];

    inv[12] = -m[4]  * m[9] * m[14] + 
               m[4]  * m[10] * m[13] +
               m[8]  * m[5] * m[14] - 
               m[8]  * m[6] * m[13] - 
               m[12] * m[5] * m[10] + 
               m[12] * m[6] * m[9];

    inv[1] = -m[1]  * m[10] * m[15] + 
              m[1]  * m[11] * m[14] + 
              m[9]  * m[2] * m[15] - 
              m[9]  * m[3] * m[14] - 
              m[13] * m[2] * m[11] + 
              m[13] * m[3] * m[10];

    inv[5] = m[0]  * m[10] * m[15] - 
             m[0]  * m[11] * m[14] - 
             m[8]  * m[2] * m[15] + 
             m[8]  * m[3] * m[14] + 
             m[12] * m[2] * m[11] - 
             m[12] * m[3] * m[10];

    inv[9] = -m[0]  * m[9] * m[15] + 
              m[0]  * m[11] * m[13] + 
              m[8]  * m[1] * m[15] - 
              m[8]  * m[3] * m[13] - 
              m[12] * m[1] * m[11] + 
              m[12] * m[3] * m[9];

    inv[13] = m[0]  * m[9] * m[14] - 
              m[0]  * m[10] * m[13] - 
              m[8]  * m[1] * m[14] + 
              m[8]  * m[2] * m[13] + 
              m[12] * m[1] * m[10] - 
              m[12] * m[2] * m[9];

    inv[2] = m[1]  * m[6] * m[15] - 
             m[1]  * m[7] * m[14] - 
             m[5]  * m[2] * m[15] + 
             m[5]  * m[3] * m[14] + 
             m[13] * m[2] * m[7] - 
             m[13] * m[3] * m[6];

    inv[6] = -m[0]  * m[6] * m[15] + 
              m[0]  * m[7] * m[14] + 
              m[4]  * m[2] * m[15] - 
              m[4]  * m[3] * m[14] - 
              m[12] * m[2] * m[7] + 
              m[12] * m[3] * m[6];

    inv[10] = m[0]  * m[5] * m[15] - 
              m[0]  * m[7] * m[13] - 
              m[4]  * m[1] * m[15] + 
              m[4]  * m[3] * m[13] + 
              m[12] * m[1] * m[7] - 
              m[12] * m[3] * m[5];

    inv[14] = -m[0]  * m[5] * m[14] + 
               m[0]  * m[6] * m[13] + 
               m[4]  * m[1] * m[14] - 
               m[4]  * m[2] * m[13] - 
               m[12] * m[1] * m[6] + 
               m[12] * m[2] * m[5];

    inv[3] = -m[1] * m[6] * m[11] + 
              m[1] * m[7] * m[10] + 
              m[5] * m[2] * m[11] - 
              m[5] * m[3] * m[10] - 
              m[9] * m[2] * m[7] + 
              m[9] * m[3] * m[6];

    inv[7] = m[0] * m[6] * m[11] - 
             m[0] * m[7] * m[10] - 
             m[4] * m[2] * m[11] + 
             m[4] * m[3] * m[10] + 
             m[8] * m[2] * m[7] - 
             m[8] * m[3] * m[6];

    inv[11] = -m[0] * m[5] * m[11] + 
               m[0] * m[7] * m[9] + 
               m[4] * m[1] * m[11] - 
               m[4] * m[3] * m[9] - 
               m[8] * m[1] * m[7] + 
               m[8] * m[3] * m[5];

    inv[15] = m[0] * m[5] * m[10] - 
              m[0] * m[6] * m[9] - 
              m[4] * m[1] * m[10] + 
              m[4] * m[2] * m[9] + 
              m[8] * m[1] * m[6] - 
              m[8] * m[2] * m[5];

    float det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];

    assert( det != 0 );

    det = 1.0 / det;

    Mat4 out;
    float* const outx = (float*)out.x;
    for (int i = 0; i < 16; i++) 
    {
        outx[i] = inv[i] * det;
    }
    return out;
}

#define EPSILON 0.000001
// Moller & Trumbore triangle intersection algorithm
// this culls all triangles facing away
int m_IntersectTriangle(const Vec3* orig, const Vec3* dir, 
        const Vec3* vert0, const Vec3* vert1, const Vec3* vert2,
        float* t, float* u, float* v)
{
    const Vec3 edge1 = m_Sub_Vec3(vert1, vert0);
    const Vec3 edge2 = m_Sub_Vec3(vert2, vert0);
    const Vec3 pvec  = m_Cross(dir, &edge2);
    const float det  = m_Dot(&edge1, &pvec);
    if (det < EPSILON) 
        return 0;
    const Vec3 tvec  = m_Sub_Vec3(orig, vert0);
    *u = m_Dot(&tvec, &pvec);
    if (*u < 0.0 || *u > det) // may have branch detection issues
        return 0; 
    const Vec3 qvec  = m_Cross(&tvec, &edge1);
    *v = m_Dot(dir, &qvec);
    if (*v < 0.0 || *u + *v > det) 
        return 0;
    *t = m_Dot(&edge2, &qvec);
    const float inv_det = 1.0 / det;
    *t *= inv_det;
    *u *= inv_det;
    *v *= inv_det;
    return 1;
}

Vec3 m_Lerp_Vec3(const Vec3* a, const Vec3* b, const float t)
{
    assert(t >= 0.0 && t <= 1.0);
    Vec3 v;
    v.x[0] = a->x[0] * (1.0 - t) + b->x[0] * t;
    v.x[1] = a->x[1] * (1.0 - t) + b->x[1] * t;
    v.x[2] = a->x[2] * (1.0 - t) + b->x[2] * t;
    return v;
}
