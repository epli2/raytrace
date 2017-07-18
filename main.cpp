#include <iostream>
#include <cmath>
#include <vector>
#include <fstream>
using namespace std;
#define MAX 50
#define INF 1.0e7
#define EPS 1.0e-3
#define SCREEN 500
const string inputfilename = "input.txt";
const string ppmfilename = "out.ppm";
enum Type { SPHERE = 0, PLANE = 1, CYLINDER = 2};
enum Op { NONE = 0, UNION = 1, SUB = 2, INTERSECT = 3 };

struct Vec3 {
  double x, y, z;
  Vec3() {};
  Vec3(double x, double y, double z) : x(x), y(y), z(z){};
  inline Vec3 operator+(const Vec3 &v) const {
    return Vec3(this->x + v.x, this->y + v.y, this->z + v.z);
  }
  inline Vec3 operator-(const Vec3 &v) const {
    return Vec3(this->x - v.x, this->y - v.y, this->z - v.z);
  }
  inline Vec3 operator*(const double s) const {
    return Vec3(this->x * s, this->y * s, this->z * s);
  }
  inline Vec3 operator/(const double s) const {
    return Vec3(this->x / s, this->y / s, this->z / s);
  }
  static double dot(const Vec3 v1, const Vec3 v2) {
    return (v1.x * v2.x + v1.y * v2.y + v1.z * v2.z);
  }
  static double length(const Vec3 v) {
    return sqrt(dot(v, v));
  }
  static Vec3 normal(const Vec3 v) {
    return v / length(v);
  }
};

struct Color {
  double r, g, b;
  Color() {};
  Color(double r, double g, double b) : r(r), g(g), b(b){};
  inline Color operator+(const Color &c) const {
    return Color(this->r + c.r, this->g + c.g, this->b + c.b);
  }
  inline Color operator+(const double s) const {
    return Color(this->r + s, this->g + s, this->b + s);
  }
  inline Color operator*(const double s) const {
    return Color(this->r * s, this->g * s, this->b * s);
  }
};

struct Obj {
  int t;
  int op;
  double r;
  Vec3 p;
  Vec3 v;
  Color color;
};

void input();
void printimg(Color img[SCREEN][SCREEN]);
void raycast(Vec3 v, Vec3 e, double *t, int *objnum, int *objtype);
Color raytrace(Vec3 v, Vec3 e);
double shadowing(Vec3 v);
Color shading(Vec3 n, int i, Vec3 v, Vec3 e);

vector<Obj> objs;
Vec3 l;

int main() {
  Color img[SCREEN][SCREEN];
  input();
  for (int y = 0; y < SCREEN; y++) {
    for (int x = 0; x < SCREEN; x++) {
      Vec3 v = Vec3(0, 100, 1500.0); // 視点座標
      Vec3 e = Vec3::normal(Vec3(x, y, 0.0) - v); // 視線ベクトル
      img[x][y] = raytrace(v, e);
    }
  }
  printimg(img);
}

void input() {
  ifstream inputfile;
  inputfile.open(inputfilename, ios::in);
  string linebuf;
  inputfile >> l.x >> l.y >> l.z;
  l = Vec3::normal(l);
  Obj obj;
  do {
    inputfile >> obj.op;
    inputfile >> obj.t;
    if (obj.t == SPHERE || obj.t == CYLINDER) {
      inputfile >> obj.r
                >> obj.p.x >> obj.p.y >> obj.p.z
                >> obj.color.r >> obj.color.g >> obj.color.b;
    } else if (obj.t == PLANE) {
      inputfile >> obj.p.x >> obj.p.y >> obj.p.z
                >> obj.v.x >> obj.v.y >> obj.v.z
                >> obj.color.r >> obj.color.g >> obj.color.b;
    }
    objs.push_back(obj);
  } while (obj.op < 9999.0);
}

void printimg(Color img[SCREEN][SCREEN]) { // ppm形式で出力
  ofstream ppmfile;
  ppmfile.open(ppmfilename, ios::out);
  ppmfile << "P3" << endl;
  ppmfile << SCREEN << " " << SCREEN << endl;
  ppmfile << 255 << endl;
  for (int y = 0; y < SCREEN; y++) {
    for (int x = 0; x < SCREEN; x++) {
      int r = img[x][y].r > 1 ? 255 : int(img[x][y].r * 255);
      int g = img[x][y].g > 1 ? 255 : int(img[x][y].g * 255);
      int b = img[x][y].b > 1 ? 255 : int(img[x][y].b * 255);
      ppmfile << r << " " << g << " " << b << endl;
    }
  }
}

void raycast(Vec3 v, Vec3 e, double *t, int *objnum, int *objtype) {
  *t = INF; // 距離を無限遠に初期化
  for (int i = 0; i < objs.size(); i++) {
    double pre_d;
    Vec3 w = v - objs[i].p;
    if (objs[i].t == SPHERE) {
      double b = 2.0 * Vec3::dot(e, w);
      double c = Vec3::dot(w, w) - objs[i].r * objs[i].r;
      double d = b * b - 4.0 * c;
      if (i >= 1) {
        if (objs[i - 1].op == SUB) {
          d = min(-pre_d, d);
        } else if (objs[i - 1].op == INTERSECT) {
          d = -max(-pre_d, -d);
        }
      }
      if (d >= 0.0 && objs[i].op == NONE) {
        double t0 = (-b - sqrt(d)) / 2.0;
        if (t0 >= 0.0 && t0 < *t) {
          *t = t0;
          *objnum = i;
          *objtype = SPHERE;
        }
      }
      pre_d = d;
    } else if (objs[i].t == PLANE) {
      double ve = Vec3::dot(objs[0].v, e);
      if (abs(ve) > 1.0e-7) {
        double t0 = -Vec3::dot(objs[0].v, w) / ve;
        if (t0 >= 0.0 && t0 < *t) {
          *t = t0;
          *objnum = 0;
          *objtype = PLANE;
        }
      }
    } else if (objs[i].t == CYLINDER) {
      double a = e.x * e.x + e.y * e.y;
      double b = 2 * (e.x * w.x + e.y * w.y);
      double c = w.x * w.x + w.y * w.y - objs[i].r * objs[i].r;
      double d = b * b - 4 * a * c;
      if (d >= 0.0 && objs[i].op == NONE) {
        double t0 = -b + sqrt(d) / (2.0 * a);
        if (t0 >= 0.0 && t0 < *t) {
          *t = t0;
          *objnum = i;
          *objtype = CYLINDER;
        }
      }
    }
  }
}

Color raytrace(Vec3 v, Vec3 e) {
  double t;
  int objnum, objtype;
  raycast(v, e, &t, &objnum, &objtype); // 交点を求める
  if (t < INF) { // 交点あり
    Vec3 tmp = e * t + v;
    Vec3 n;
    if (objtype == PLANE) {
      n = objs[objnum].v;
    } else if (objtype == CYLINDER) {
      n.x = tmp.x - objs[objnum].p.x;
      n.y = tmp.y - objs[objnum].p.y;
      n.z = 0;
      n = Vec3::normal(n);
    } else {
      n = Vec3::normal(tmp - objs[objnum].p);
    }
    return shading(n, objnum, tmp, e);
  }
  return Color(0.0, 0.0, 0.0); // 交点なし
}

double shadowing(Vec3 v) {
  double br = 1.0, t;
  int objnum, objtype;
  raycast(v + l * EPS, l, &t, &objnum, &objtype);
  if (t < INF) {
    br = 0.25;
  }
  return br;
}

Color shading(Vec3 n, int i, Vec3 v, Vec3 e) {
  double ln = Vec3::dot(l, n);
  Vec3 r = n * ln * 2 - l;
  double lv = -Vec3::dot(r, e);
  lv = (lv > 0.0) ? pow(lv, 50) : 0.0;
  ln = (ln < 0.0) ? 0.0 : ln;
  double br = shadowing(v);
  return objs[i].color * ln * br + lv * br;
}
