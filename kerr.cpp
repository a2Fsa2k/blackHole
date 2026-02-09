#include <cmath>
#include <fstream>
#include <iostream>
#include <algorithm>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

constexpr int WIDTH = 1280;
constexpr int HEIGHT = 720;
constexpr double PI = 3.1415926538;

// Global space background texture
unsigned char* spaceImage = nullptr;
int spaceWidth = 0, spaceHeight = 0, spaceChannels = 0;

// Settings
double camR = 30.0;
double tilt = 0.05;  // Very small tilt - almost edge-on view (Interstellar!)
double zoom = 1.0;
double a = 0.6;
double discMin = 3.83;
double discMax = 15.0;
double eps = 0.01;
double dtau = 0.1;
int maxSteps = 500;

struct Vec3 {
    double x, y, z;
    Vec3 operator+(const Vec3& b) const { return {x+b.x, y+b.y, z+b.z}; }
    Vec3 operator-(const Vec3& b) const { return {x-b.x, y-b.y, z-b.z}; }
    Vec3 operator*(double s) const { return {x*s, y*s, z*s}; }
};

struct Vec4 {
    double t, x, y, z;
    Vec4 operator+(const Vec4& b) const { return {t+b.t, x+b.x, y+b.y, z+b.z}; }
    Vec4 operator-(const Vec4& b) const { return {t-b.t, x-b.x, y-b.y, z-b.z}; }
    Vec4 operator*(double s) const { return {t*s, x*s, y*s, z*s}; }
    Vec4 operator/(double s) const { return {t/s, x/s, y/s, z/s}; }
    Vec4& operator+=(const Vec4& b) { t+=b.t; x+=b.x; y+=b.y; z+=b.z; return *this; }
    Vec4& operator-=(const Vec4& b) { t-=b.t; x-=b.x; y-=b.y; z-=b.z; return *this; }
};

struct Mat4 {
    double m[4][4];
};

double dot(const Vec3& a, const Vec3& b) {
    return a.x*b.x + a.y*b.y + a.z*b.z;
}

double dot4(const Vec4& a, const Vec4& b) {
    return a.t*b.t + a.x*b.x + a.y*b.y + a.z*b.z;
}

Vec3 cross(const Vec3& a, const Vec3& b) {
    return {a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x};
}

double length(const Vec3& v) {
    return std::sqrt(dot(v, v));
}

Vec3 normalize(const Vec3& v) {
    double len = length(v);
    return v * (1.0 / len);
}

Mat4 diag(const Vec4& v) {
    Mat4 result = {};
    result.m[0][0] = v.t;
    result.m[1][1] = v.x;
    result.m[2][2] = v.y;
    result.m[3][3] = v.z;
    return result;
}

Mat4 outerProduct(const Vec4& a, const Vec4& b) {
    Mat4 result;
    for(int i=0; i<4; i++)
        for(int j=0; j<4; j++)
            result.m[i][j] = (&a.t)[i] * (&b.t)[j];
    return result;
}

Mat4 operator+(const Mat4& a, const Mat4& b) {
    Mat4 result;
    for(int i=0; i<4; i++)
        for(int j=0; j<4; j++)
            result.m[i][j] = a.m[i][j] + b.m[i][j];
    return result;
}

Mat4 operator*(const Mat4& a, double s) {
    Mat4 result;
    for(int i=0; i<4; i++)
        for(int j=0; j<4; j++)
            result.m[i][j] = a.m[i][j] * s;
    return result;
}

Vec4 operator*(const Mat4& m, const Vec4& v) {
    return {
        m.m[0][0]*v.t + m.m[0][1]*v.x + m.m[0][2]*v.y + m.m[0][3]*v.z,
        m.m[1][0]*v.t + m.m[1][1]*v.x + m.m[1][2]*v.y + m.m[1][3]*v.z,
        m.m[2][0]*v.t + m.m[2][1]*v.x + m.m[2][2]*v.y + m.m[2][3]*v.z,
        m.m[3][0]*v.t + m.m[3][1]*v.x + m.m[3][2]*v.y + m.m[3][3]*v.z
    };
}

Mat4 inverse(const Mat4& A) {
    double m[16], invOut[16];
    for(int i=0; i<4; i++)
        for(int j=0; j<4; j++)
            m[i*4+j] = A.m[i][j];
    
    invOut[0] = m[5]*m[10]*m[15]-m[5]*m[11]*m[14]-m[9]*m[6]*m[15]+m[9]*m[7]*m[14]+m[13]*m[6]*m[11]-m[13]*m[7]*m[10];
    invOut[4] = -m[4]*m[10]*m[15]+m[4]*m[11]*m[14]+m[8]*m[6]*m[15]-m[8]*m[7]*m[14]-m[12]*m[6]*m[11]+m[12]*m[7]*m[10];
    invOut[8] = m[4]*m[9]*m[15]-m[4]*m[11]*m[13]-m[8]*m[5]*m[15]+m[8]*m[7]*m[13]+m[12]*m[5]*m[11]-m[12]*m[7]*m[9];
    invOut[12] = -m[4]*m[9]*m[14]+m[4]*m[10]*m[13]+m[8]*m[5]*m[14]-m[8]*m[6]*m[13]-m[12]*m[5]*m[10]+m[12]*m[6]*m[9];
    invOut[1] = -m[1]*m[10]*m[15]+m[1]*m[11]*m[14]+m[9]*m[2]*m[15]-m[9]*m[3]*m[14]-m[13]*m[2]*m[11]+m[13]*m[3]*m[10];
    invOut[5] = m[0]*m[10]*m[15]-m[0]*m[11]*m[14]-m[8]*m[2]*m[15]+m[8]*m[3]*m[14]+m[12]*m[2]*m[11]-m[12]*m[3]*m[10];
    invOut[9] = -m[0]*m[9]*m[15]+m[0]*m[11]*m[13]+m[8]*m[1]*m[15]-m[8]*m[3]*m[13]-m[12]*m[1]*m[11]+m[12]*m[3]*m[9];
    invOut[13] = m[0]*m[9]*m[14]-m[0]*m[10]*m[13]-m[8]*m[1]*m[14]+m[8]*m[2]*m[13]+m[12]*m[1]*m[10]-m[12]*m[2]*m[9];
    invOut[2] = m[1]*m[6]*m[15]-m[1]*m[7]*m[14]-m[5]*m[2]*m[15]+m[5]*m[3]*m[14]+m[13]*m[2]*m[7]-m[13]*m[3]*m[6];
    invOut[6] = -m[0]*m[6]*m[15]+m[0]*m[7]*m[14]+m[4]*m[2]*m[15]-m[4]*m[3]*m[14]-m[12]*m[2]*m[7]+m[12]*m[3]*m[6];
    invOut[10] = m[0]*m[5]*m[15]-m[0]*m[7]*m[13]-m[4]*m[1]*m[15]+m[4]*m[3]*m[13]+m[12]*m[1]*m[7]-m[12]*m[3]*m[5];
    invOut[14] = -m[0]*m[5]*m[14]+m[0]*m[6]*m[13]+m[4]*m[1]*m[14]-m[4]*m[2]*m[13]-m[12]*m[1]*m[6]+m[12]*m[2]*m[5];
    invOut[3] = -m[1]*m[6]*m[11]+m[1]*m[7]*m[10]+m[5]*m[2]*m[11]-m[5]*m[3]*m[10]-m[9]*m[2]*m[7]+m[9]*m[3]*m[6];
    invOut[7] = m[0]*m[6]*m[11]-m[0]*m[7]*m[10]-m[4]*m[2]*m[11]+m[4]*m[3]*m[10]+m[8]*m[2]*m[7]-m[8]*m[3]*m[6];
    invOut[11] = -m[0]*m[5]*m[11]+m[0]*m[7]*m[9]+m[4]*m[1]*m[11]-m[4]*m[3]*m[9]-m[8]*m[1]*m[7]+m[8]*m[3]*m[5];
    invOut[15] = m[0]*m[5]*m[10]-m[0]*m[6]*m[9]-m[4]*m[1]*m[10]+m[4]*m[2]*m[9]+m[8]*m[1]*m[6]-m[8]*m[2]*m[5];
    
    double invDet = m[0]*invOut[0] + m[1]*invOut[4] + m[2]*invOut[8] + m[3]*invOut[12];
    invDet = 1.0 / invDet;
    
    Mat4 inv;
    for(int i=0; i<4; i++)
        for(int j=0; j<4; j++)
            inv.m[i][j] = invOut[i*4+j] * invDet;
    return inv;
}

double rFromCoords(const Vec4& pos) {
    Vec3 p = {pos.x, pos.y, pos.z};
    double rho2 = dot(p, p) - a*a;
    double r2 = 0.5 * (rho2 + std::sqrt(rho2*rho2 + 4.0*a*a*pos.z*pos.z));
    return std::sqrt(r2);
}

Mat4 metric(const Vec4& pos) {
    double r = rFromCoords(pos);
    Vec4 k = {-1.0, (r*pos.x - a*pos.y)/(r*r+a*a), (r*pos.y + a*pos.x)/(r*r+a*a), pos.z/r};
    double f = 2.0*r / (r*r + a*a*pos.z*pos.z/(r*r));
    return outerProduct(k, k) * f + diag({-1, 1, 1, 1});
}

double hamiltonian(const Vec4& x, const Vec4& p) {
    return 0.5 * dot4(inverse(metric(x)) * p, p);
}

Vec4 hamiltonianGradient(const Vec4& x, const Vec4& p) {
    double h0 = hamiltonian(x, p);
    return Vec4{
        hamiltonian(x + Vec4{eps,0,0,0}, p),
        hamiltonian(x + Vec4{0,eps,0,0}, p),
        hamiltonian(x + Vec4{0,0,eps,0}, p),
        hamiltonian(x + Vec4{0,0,0,eps}, p)
    } * (1.0/eps) - Vec4{h0/eps, h0/eps, h0/eps, h0/eps};
}

void transportStep(Vec4& x, Vec4& p) {
    p -= hamiltonianGradient(x, p) * dtau;
    x += inverse(metric(x)) * p * dtau;
}

bool stopCondition(const Vec4& pos) {
    double r = rFromCoords(pos);
    return r < 1.0 + std::sqrt(1.0 - a*a) || r > std::max(2.0*camR, 30.0);
}

// Realistic space background from texture
Vec3 skyTexture(const Vec3& dir) {
    if (!spaceImage) {
        // Fallback to checkerboard if image failed to load
        double u = 0.5 + std::atan2(dir.y, dir.x) / (2.0*PI);
        double v = 0.5 - std::asin(dir.z) / PI;
        int check = (int(std::floor(u*20)) ^ int(std::floor(v*10))) & 1;
        return check ? Vec3{0.9,0.9,0.9} : Vec3{0.1,0.1,0.1};
    }
    
    // Convert 3D direction to spherical coordinates
    double u = 0.5 + std::atan2(dir.y, dir.x) / (2.0 * PI);
    double v = 0.5 - std::asin(std::fmax(-1.0, std::fmin(1.0, dir.z))) / PI;
    
    // Sample texture
    int px = int(u * spaceWidth) % spaceWidth;
    int py = int(v * spaceHeight) % spaceHeight;
    
    if (px < 0) px += spaceWidth;
    if (py < 0) py += spaceHeight;
    
    int idx = (py * spaceWidth + px) * spaceChannels;
    
    return Vec3{
        spaceImage[idx + 0] / 255.0,
        spaceImage[idx + 1] / 255.0,
        spaceImage[idx + 2] / 255.0
    };
}

// Procedural disk texture
Vec3 discTexture(double u, double v) {
    double r = std::sqrt((u-0.5)*(u-0.5) + (v-0.5)*(v-0.5)) * 2.0;
    double angle = std::atan2(v-0.5, u-0.5);
    int spirals = int(std::floor(angle * 3.0 / PI + r * 10.0)) & 1;
    Vec3 color = spirals ? Vec3{1.0, 0.6, 0.2} : Vec3{0.8, 0.4, 0.1};
    return color * (1.0 - r * 0.5);
}

Vec3 mainImage(int px, int py, double iTime) {
    double u = (2.0*px - WIDTH) / (double)WIDTH;
    double v = (2.0*py - HEIGHT) / (double)WIDTH;
    
    // tilt = std::sin(iTime * 0.5) * 0.5;  // Disabled animation, using fixed tilt
    
    double x = std::sqrt(camR*camR + a*a) * std::cos(tilt);
    double z = camR * std::sin(tilt);
    Vec4 camPos = {0.0, x, 0.0, z};
    
    // Camera aim direction - point towards black hole at origin
    Vec3 toCenter = normalize(Vec3{-x, 0.0, -z});
    
    // Build camera basis
    Vec3 camRight = normalize(Vec3{0.0, 1.0, 0.0});  // y-axis
    Vec3 camUp = normalize(cross(camRight, toCenter));
    camRight = cross(toCenter, camUp);  // Re-orthogonalize
    
    // Ray direction in camera space
    Vec3 dir = normalize(toCenter * zoom + camRight * u + camUp * v);
    Vec4 dir4D = {-1.0, dir.x, dir.y, dir.z};
    
    Vec4 pos = camPos;
    Vec4 p = metric(pos) * dir4D;
    
    bool captured = false;
    bool hitDisc = false;
    double discU = 0, discV = 0;
    double blueshift = 1.0;
    
    for(int i=0; i<maxSteps; i++) {
        Vec4 lastpos = pos;
        transportStep(pos, p);
        
        if(pos.z * lastpos.z < 0.0) {
            Vec4 intersectPos = (pos * std::abs(lastpos.z) + lastpos * std::abs(pos.z)) / 
                                std::abs(lastpos.z - pos.z);
            double r = rFromCoords(intersectPos);
            if(r > discMin && r < discMax) {
                hitDisc = true;
                discU = (intersectPos.x / discMax + 1.0) * 0.5;
                discV = (intersectPos.y / discMax + 1.0) * 0.5;
                
                Vec4 discVel = {r + a/std::sqrt(r), 
                               -intersectPos.y * (a >= 0 ? 1 : -1) / std::sqrt(r),
                               intersectPos.x * (a >= 0 ? 1 : -1) / std::sqrt(r), 
                               0.0};
                discVel = discVel * (1.0 / std::sqrt(r*r - 3.0*r + 2.0*a*std::sqrt(r)));
                blueshift = 1.0 / dot4(p, discVel);
                break;
            }
        }
        
        if(stopCondition(pos)) {
            double r = rFromCoords(pos);
            captured = r < 1.0 + std::sqrt(1.0 - a*a);
            break;
        }
    }
    
    if(hitDisc) {
        Vec3 color = discTexture(discU, discV);
        return color * std::pow(std::abs(blueshift), 3.0);
    } else if(!captured) {
        Vec4 dir4D_out = inverse(metric(pos)) * p;
        Vec3 cubeVec = {-dir4D_out.x, dir4D_out.z, -dir4D_out.y};
        return skyTexture(cubeVec);
    } else {
        return {0.0, 0.0, 0.0};
    }
}

int main() {
    // Load space background image
    spaceImage = stbi_load("space_background.jpg", &spaceWidth, &spaceHeight, &spaceChannels, 3);
    if (!spaceImage) {
        std::cerr << "Warning: Failed to load space_background.jpg, using fallback\n";
    } else {
        std::cout << "Loaded space background: " << spaceWidth << "x" << spaceHeight << "\n";
    }
    
    std::ofstream out("kerr_hamiltonian.ppm", std::ios::binary);
    out << "P6\n" << WIDTH << " " << HEIGHT << "\n255\n";
    
    double iTime = 0.0;
    
    for(int y=HEIGHT-1; y>=0; y--) {
        for(int x=0; x<WIDTH; x++) {
            Vec3 col = mainImage(x, y, iTime);
            unsigned char r = (unsigned char)(std::fmin(std::fmax(col.x, 0.0), 1.0) * 255);
            unsigned char g = (unsigned char)(std::fmin(std::fmax(col.y, 0.0), 1.0) * 255);
            unsigned char b = (unsigned char)(std::fmin(std::fmax(col.z, 0.0), 1.0) * 255);
            out << r << g << b;
        }
    }
    
    out.close();
    
    // Free image memory
    if (spaceImage) {
        stbi_image_free(spaceImage);
    }
    
    std::cout << "Rendered kerr_hamiltonian.ppm\n";
    return 0;
}
