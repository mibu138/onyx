#ifndef M_MATH_H
#define M_MATH_H

#include <math.h>

typedef struct {
    float x;
    float y;
} Vec2;

typedef struct {
    float x[3];
} Vec3;

typedef struct {
    float x[4];
} Vec4;

typedef struct {
    float x00, x01, 
          x10, x11;
} Mat2;

typedef struct {
    float x[3][3];
} Mat3;

typedef struct {
    float x[4][4];
} Mat4;

typedef struct {
    Vec2 orig;
    Vec2 dir;
} Ray;

typedef struct {
    Vec2 A;
    Vec2 B;
} Segment;

Mat4 m_Ident_Mat4(void);
Mat4 m_BuildPerspective(const float nearDist, const float farDist);
Vec3 m_GetLocalZ_Mat4(const Mat4* m);
Vec3 m_GetTranslation_Mat4(const Mat4* m);
Vec3 m_Add_Vec3(const Vec3* a, const Vec3* b);
Vec3 m_Sub_Vec3(const Vec3* a, const Vec3* b);
Vec3 m_Normalize_Vec3(const Vec3* v);
Vec3 m_Scale_Vec3(const float s, const Vec3* v);
Vec3 m_Cross(const Vec3* a, const Vec3* b);
Mat4 m_LookAt(const Vec3* pos, const Vec3* target, const Vec3* up);
void m_Rotate_Vec2(const float angle /* radians */, Vec2*);
Mat4 m_RotateY_Mat4(const float angle, const Mat4* m);
Mat4 m_BuildFromBasis_Mat4(const float x[3], const float y[3], const float z[3]);
Mat4 m_BuildRotate(const float angle, const Vec3* axis);
Vec3 m_RotateY_Vec3(const float angle, const Vec3* v);
Mat4 m_RotateZ_Mat4(const float angle, const Mat4* m);
Mat4 m_Mult_Mat4(const Mat4* a, const Mat4* b);
Vec3 m_Mult_Mat4Vec3(const Mat4* m, const Vec3* v);
Mat4 m_Translate_Mat4(const Vec3 t, const Mat4* m);
Mat4 m_Transpose_Mat4(const Mat4* m);
void m_ScaleUniform_Mat4(const float s, Mat4* m);
void m_Translate(const Vec2 t, Vec2*);
void m_Scale(const float scale, Vec2*);
void m_Add(const Vec2, Vec2*);
Vec2 m_Subtract(const Vec2, const Vec2);
//returns a random float between 0 and 1
float m_Rand(void); 
//returns a random float between -1 and 1
float m_RandNeg(void); 
float m_Length(const Vec2);
float m_Length2(const Vec2);
float m_Determinant(const Mat2);
Vec2  m_PolarToCart(const float angle, const float radius);
Mat4  m_Invert4x4(const Mat4* const mat);

#endif /* end of include guard: M_MATH_H */
