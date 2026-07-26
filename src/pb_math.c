#include "playbox/pb_math.h"
#include <string.h>

pb_mat4 pb_m4_identity(void){
    pb_mat4 m;
    memset(m.m, 0, sizeof(m.m));
    m.m[0] = m.m[5] = m.m[10] = m.m[15] = 1.0f;
    return m;
}

pb_mat4 pb_m4_translate(pb_vec3 t){
    pb_mat4 m = pb_m4_identity();
    m.m[12] = t.x; m.m[13] = t.y; m.m[14] = t.z;
    return m;
}

pb_mat4 pb_m4_scale(pb_vec3 s){
    pb_mat4 m = pb_m4_identity();
    m.m[0] = s.x; m.m[5] = s.y; m.m[10] = s.z;
    return m;
}

pb_mat4 pb_m4_rotate_x(float rad){
    pb_mat4 m = pb_m4_identity();
    float c = cosf(rad), s = sinf(rad);
    m.m[5] = c;  m.m[6] = s;
    m.m[9] = -s; m.m[10] = c;
    return m;
}

pb_mat4 pb_m4_rotate_y(float rad){
    pb_mat4 m = pb_m4_identity();
    float c = cosf(rad), s = sinf(rad);
    m.m[0] = c;  m.m[2] = -s;
    m.m[8] = s;  m.m[10] = c;
    return m;
}

pb_mat4 pb_m4_rotate_z(float rad){
    pb_mat4 m = pb_m4_identity();
    float c = cosf(rad), s = sinf(rad);
    m.m[0] = c;  m.m[1] = s;
    m.m[4] = -s; m.m[5] = c;
    return m;
}

pb_mat4 pb_m4_mul(pb_mat4 a, pb_mat4 b){
    pb_mat4 r;
    for(int col = 0; col < 4; col++){
        for(int row = 0; row < 4; row++){
            r.m[col*4 + row] =
                a.m[0*4 + row] * b.m[col*4 + 0] +
                a.m[1*4 + row] * b.m[col*4 + 1] +
                a.m[2*4 + row] * b.m[col*4 + 2] +
                a.m[3*4 + row] * b.m[col*4 + 3];
        }
    }
    return r;
}

pb_vec4 pb_m4_mul_v4(pb_mat4 m, pb_vec4 v){
    return pb_v4(
        m.m[0]*v.x + m.m[4]*v.y + m.m[8]*v.z  + m.m[12]*v.w,
        m.m[1]*v.x + m.m[5]*v.y + m.m[9]*v.z  + m.m[13]*v.w,
        m.m[2]*v.x + m.m[6]*v.y + m.m[10]*v.z + m.m[14]*v.w,
        m.m[3]*v.x + m.m[7]*v.y + m.m[11]*v.z + m.m[15]*v.w
    );
}

pb_vec3 pb_m4_mul_v3(pb_mat4 m, pb_vec3 v, float w){
    pb_vec4 r = pb_m4_mul_v4(m, pb_v4(v.x, v.y, v.z, w));
    if(w != 0.0f && fabsf(r.w) > 1e-8f){
        float iw = 1.0f / r.w;
        return pb_v3(r.x * iw, r.y * iw, r.z * iw);
    }
    return pb_v3(r.x, r.y, r.z);
}

pb_mat4 pb_m4_look_at(pb_vec3 eye, pb_vec3 target, pb_vec3 up){
    pb_vec3 f = pb_v3_normalize(pb_v3_sub(target, eye));
    pb_vec3 s = pb_v3_normalize(pb_v3_cross(f, up));
    pb_vec3 u = pb_v3_cross(s, f);

    pb_mat4 m = pb_m4_identity();
    m.m[0] = s.x;  m.m[4] = s.y;  m.m[8]  = s.z;
    m.m[1] = u.x;  m.m[5] = u.y;  m.m[9]  = u.z;
    m.m[2] = -f.x; m.m[6] = -f.y; m.m[10] = -f.z;
    m.m[12] = -pb_v3_dot(s, eye);
    m.m[13] = -pb_v3_dot(u, eye);
    m.m[14] =  pb_v3_dot(f, eye);
    return m;
}

pb_mat4 pb_m4_perspective(float fovy_rad, float aspect, float znear, float zfar){
    pb_mat4 m;
    memset(m.m, 0, sizeof(m.m));
    float f = 1.0f / tanf(fovy_rad * 0.5f);
    m.m[0] = f / aspect;
    m.m[5] = f;
    m.m[10] = (zfar + znear) / (znear - zfar);
    m.m[11] = -1.0f;
    m.m[14] = (2.0f * zfar * znear) / (znear - zfar);
    return m;
}

pb_mat4 pb_m4_ortho(float left, float right, float bottom, float top, float znear, float zfar){
    pb_mat4 m = pb_m4_identity();
    m.m[0] = 2.0f / (right - left);
    m.m[5] = 2.0f / (top - bottom);
    m.m[10] = -2.0f / (zfar - znear);
    m.m[12] = -(right + left) / (right - left);
    m.m[13] = -(top + bottom) / (top - bottom);
    m.m[14] = -(zfar + znear) / (zfar - znear);
    return m;
}

int pb_math_project(pb_mat4 vp, pb_vec3 world,
                    int pixel_w, int pixel_h,
                    int* out_px, int* out_py, float* out_z){
    pb_vec4 clip = pb_m4_mul_v4(vp, pb_v4(world.x, world.y, world.z, 1.0f));
    if(clip.w <= 1e-6f) return 0;
    float iw = 1.0f / clip.w;
    float ndc_x = clip.x * iw;
    float ndc_y = clip.y * iw;
    float ndc_z = clip.z * iw;
    if(out_px) *out_px = (int)floorf((ndc_x * 0.5f + 0.5f) * (float)pixel_w);
    if(out_py) *out_py = (int)floorf((1.0f - (ndc_y * 0.5f + 0.5f)) * (float)pixel_h);
    if(out_z)  *out_z  = ndc_z * 0.5f + 0.5f;
    return 1;
}
