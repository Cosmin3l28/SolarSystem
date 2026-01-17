/*
    SISTEM SOLAR 
    -------------------------------------------------------------
    Simulare interactiva a sistemului solar.
*/

#include <windows.h>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <vector>
#include <cmath>
#include <string>
#include <iostream>
#include <iomanip>

// Biblioteci necesare pentru Windows
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glu32.lib")

#define PI 3.14159265359f
#define DEG2RAD(x) ((x) * PI / 180.0f) // Conversie Grade -> Radiani

struct Vec3 { float x, y, z; };

struct Mat4 {
    float m[16]; // Matrice 4x4 liniarizata

    // Matricea "nula" (nu modifica nimic)
    static Mat4 Identity() { Mat4 r = { 0 }; r.m[0] = 1; r.m[5] = 1; r.m[10] = 1; r.m[15] = 1; return r; }

    // Inmultirea matricilor (combina rotatii, mutari etc.)
    Mat4 operator*(const Mat4& b) const {
        Mat4 r = { 0 };
        for (int c = 0; c < 4; c++) for (int rw = 0; rw < 4; rw++)
            r.m[c * 4 + rw] = m[0 * 4 + rw] * b.m[c * 4 + 0] + m[1 * 4 + rw] * b.m[c * 4 + 1] + m[2 * 4 + rw] * b.m[c * 4 + 2] + m[3 * 4 + rw] * b.m[c * 4 + 3];
        return r;
    }

    // Creaza efectul de perspectiva (obiectele indepartate se vad mici)
    static Mat4 Perspective(float fov, float ar, float n, float f) {
        Mat4 r = { 0 }; float t = tan(fov / 2);
        r.m[0] = 1 / (ar * t); r.m[5] = 1 / t; r.m[10] = -(f + n) / (f - n); r.m[11] = -1; r.m[14] = -(2 * f * n) / (f - n); return r;
    }

    // Pozitioneaza camera (Ochiul, Tinta, Directia 'Sus')
    static Mat4 LookAt(Vec3 eye, Vec3 center, Vec3 up) {
        auto n = [](Vec3 v) {float l = sqrt(v.x * v.x + v.y * v.y + v.z * v.z); return Vec3{ v.x / l,v.y / l,v.z / l }; };
        auto c = [](Vec3 a, Vec3 b) {return Vec3{ a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x }; };
        auto d = [](Vec3 a, Vec3 b) {return a.x * b.x + a.y * b.y + a.z * b.z; };
        Vec3 f = n({ center.x - eye.x,center.y - eye.y,center.z - eye.z });
        Vec3 s = n(c(f, up)); Vec3 u = c(s, f);
        Mat4 r = Identity();
        r.m[0] = s.x; r.m[4] = s.y; r.m[8] = s.z;
        r.m[1] = u.x; r.m[5] = u.y; r.m[9] = u.z;
        r.m[2] = -f.x; r.m[6] = -f.y; r.m[10] = -f.z;
        r.m[12] = -d(s, eye); r.m[13] = -d(u, eye); r.m[14] = d(f, eye);
        return r;
    }

    // Functii standard: Mutare, Scalare, Rotire
    static Mat4 Translate(float x, float y, float z) { Mat4 r = Identity(); r.m[12] = x; r.m[13] = y; r.m[14] = z; return r; }
    static Mat4 Scale(float x, float y, float z) { Mat4 r = Identity(); r.m[0] = x; r.m[5] = y; r.m[10] = z; return r; }
    static Mat4 RotateY(float a) { Mat4 r = Identity(); float c = cos(a), s = sin(a); r.m[0] = c; r.m[8] = s; r.m[2] = -s; r.m[10] = c; return r; }
    static Mat4 RotateX(float a) { Mat4 r = Identity(); float c = cos(a), s = sin(a); r.m[5] = c; r.m[9] = -s; r.m[6] = s; r.m[10] = c; return r; }
};

// Vertex Shader: Pune punctele 3D pe ecranul 2D
const char* vsSrc = R"(
    #version 330 core
    layout(location=0) in vec3 aPos;
    layout(location=1) in vec3 aNormal;
    out vec3 FragPos; out vec3 Normal; out vec3 LocalPos;
    uniform mat4 model; uniform mat4 view; uniform mat4 projection;
    void main(){
        FragPos=vec3(model*vec4(aPos,1.0));
        Normal=mat3(transpose(inverse(model)))*aNormal;
        LocalPos=aPos;
        gl_Position=projection*view*vec4(FragPos,1.0);
    }
)";

// Fragment Shader: Coloreaza pixelii (lumina + texturi generate)
const char* fsSrc = R"(
    #version 330 core
    out vec4 FragColor;
    in vec3 FragPos; in vec3 Normal; in vec3 LocalPos;
    uniform vec3 baseColor; uniform vec3 lightPos; uniform vec3 viewPos; uniform int type;
    void main(){
        vec3 col = baseColor;
        
        // Generam suprafata planetelor din cod (Procedural)
        if(type==2) { // Jupiter/Saturn - dungi orizontale
            float b = sin(LocalPos.y*20.0)*0.1 + sin(LocalPos.x*5+LocalPos.y*10)*0.02;
            col = mix(baseColor, baseColor*0.6, smoothstep(0.4,0.6, b+0.5));
        }
        if(type==4) { // Pamant - continente simulate
            float c = sin(LocalPos.x*4)*cos(LocalPos.z*4) + sin(LocalPos.y*5)*0.5;
            if(c>0.1) col=vec3(0.1,0.5,0.2); // Verde
            else col=vec3(0.0,0.2,0.7);      // Albastru
        }
        
        // Soarele emite lumina, nu primeste umbre
        if(type==0) { FragColor=vec4(col,1); return; } 

        // Calculam lumina (Model Phong: Ambient + Diffuz + Specular)
        vec3 N = normalize(Normal);
        vec3 L = normalize(lightPos - FragPos);
        float diff = max(dot(N, L), 0.0);
        
        // Stralucirea de pe suprafata
        vec3 spec = vec3(0);
        if(diff > 0) {
            vec3 V = normalize(viewPos - FragPos);
            vec3 R = reflect(-L, N);
            spec = vec3(0.4) * pow(max(dot(V, R), 0.0), 32);
        }
        
        // Efect de atmosfera la margini
        float rim = 1.0 - max(dot(normalize(viewPos-FragPos), N), 0.0);
        vec3 atm = vec3(0.3,0.4,0.6) * smoothstep(0.6, 1.0, rim) * 0.5;
        
        FragColor = vec4((vec3(0.1) + diff + spec + atm) * col, 1.0);
    }
)";

GLuint prog; //  Shader
GLuint sphereVAO, sphereCnt; //  sfera
GLuint starVAO, starCnt;     //  stele
GLuint astVAO, astCnt;       //  asteroizi
GLuint orbitVAO, ringVAO, ringCnt; // Orbite si inele

int wW = 1200, wH = 800; // Rezolutie fereastra
float camDist = 140.0f, camAX = 25.0f, camAY = 0.0f; // Camera
float speed = 1.0f, tm = 0.0f; // Control timp
bool paused = false, drag = false;
int lMX, lMY, sel = -1; // Mouse si selectie

Mat4 matView, matProj;

// Structura cu datele unei planete
struct Planet {
    std::string name;
    float dist, rad, spd; // Distanta, raza, viteza
    float r, g, b;        // Culoare
    int type;             // Tip shader
    bool hasRings;
    float ringInc;        // Inclinare inel
    std::string i1, i2;   // Text info
    float cx, cy, cz;     // Pozitie curenta
};
std::vector<Planet> sys;

// Compilarea shaderelor
GLuint mkS(GLenum t, const char* s) {
    GLuint h = glCreateShader(t); glShaderSource(h, 1, &s, 0); glCompileShader(h);
    int o; glGetShaderiv(h, GL_COMPILE_STATUS, &o);
    if (!o) { char l[512]; glGetShaderInfoLog(h, 512, 0, l); std::cout << "Err:" << l << std::endl; }
    return h;
}

// Creaza un inel
void mkRing() {
    std::vector<float> v;
    const int S = 60;
    for (int i = 0; i <= S; i++) {
        float a = (float)i / S * 2 * PI;
        float c = cos(a), s = sin(a);
        // Zig-zag de triunghiuri pentru a forma discul
        v.push_back(c * 1.3f); v.push_back(0); v.push_back(s * 1.3f); v.push_back(0); v.push_back(1); v.push_back(0);
        v.push_back(c * 2.2f); v.push_back(0); v.push_back(s * 2.2f); v.push_back(0); v.push_back(1); v.push_back(0);
    }
    ringCnt = v.size() / 6;
    GLuint vbo; glGenVertexArrays(1, &ringVAO); glGenBuffers(1, &vbo);
    glBindVertexArray(ringVAO); glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, v.size() * 4, &v[0], GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, 0, 24, 0); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, 0, 24, (void*)12); glEnableVertexAttribArray(1);
}

// Creaza o sfera
void mkSphere() {
    std::vector<float> v; std::vector<unsigned int> i; int X = 60, Y = 60;
    for (int y = 0; y <= Y; y++) for (int x = 0; x <= X; x++) {
        float xs = (float)x / X, ys = (float)y / Y;
        float xp = cos(xs * 2 * PI) * sin(ys * PI), yp = cos(ys * PI), zp = sin(xs * 2 * PI) * sin(ys * PI);
        v.push_back(xp); v.push_back(yp); v.push_back(zp); v.push_back(xp); v.push_back(yp); v.push_back(zp);
    }
    for (int y = 0; y < Y; y++) for (int x = 0; x < X; x++) {
        i.push_back((y + 1) * (X + 1) + x); i.push_back(y * (X + 1) + x); i.push_back(y * (X + 1) + x + 1);
        i.push_back((y + 1) * (X + 1) + x); i.push_back(y * (X + 1) + x + 1); i.push_back((y + 1) * (X + 1) + x + 1);
    }
    sphereCnt = i.size();
    GLuint vbo, ebo; glGenVertexArrays(1, &sphereVAO); glGenBuffers(1, &vbo); glGenBuffers(1, &ebo);
    glBindVertexArray(sphereVAO); glBindBuffer(GL_ARRAY_BUFFER, vbo); glBufferData(GL_ARRAY_BUFFER, v.size() * 4, &v[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo); glBufferData(GL_ELEMENT_ARRAY_BUFFER, i.size() * 4, &i[0], GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, 0, 24, 0); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, 0, 24, (void*)12); glEnableVertexAttribArray(1);
}

// Genereaza stele random in spatiu
void mkStars() {
    std::vector<float> d; starCnt = 2000;
    for (int i = 0; i < starCnt; i++) {
        float r = 450 + (rand() % 200), th = (rand() % 360) * PI / 180, ph = (rand() % 180) * PI / 180;
        d.push_back(r * sin(ph) * cos(th)); d.push_back(r * sin(ph) * sin(th)); d.push_back(r * cos(ph));
        d.push_back(0); d.push_back(1); d.push_back(0);
    }
    GLuint vbo; glGenVertexArrays(1, &starVAO); glGenBuffers(1, &vbo); glBindVertexArray(starVAO);
    glBindBuffer(GL_ARRAY_BUFFER, vbo); glBufferData(GL_ARRAY_BUFFER, d.size() * 4, &d[0], GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, 0, 24, 0); glEnableVertexAttribArray(0);
}

// Genereaza centura de asteroizi
void mkAst() {
    std::vector<float> d;
    astCnt = 1500;
    for (int i = 0; i < astCnt; i++) {
        float angle = (rand() % 360) * PI / 180.0f;
        // Ii plasam intre orbita 40 si 50
        float dist = 40.0f + (rand() % 100) / 10.0f;
        float height = ((rand() % 40) - 20) / 10.0f;

        d.push_back(cos(angle) * dist);
        d.push_back(height);
        d.push_back(sin(angle) * dist);

        d.push_back(0); d.push_back(1); d.push_back(0);
    }
    GLuint vbo; glGenVertexArrays(1, &astVAO); glGenBuffers(1, &vbo); glBindVertexArray(astVAO);
    glBindBuffer(GL_ARRAY_BUFFER, vbo); glBufferData(GL_ARRAY_BUFFER, d.size() * 4, &d[0], GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, 0, 24, 0); glEnableVertexAttribArray(0);
}

// Deseneaza linia orbitei
void mkOrbit() {
    std::vector<float> d; for (int i = 0; i <= 360; i++) { float r = i * PI / 180; d.push_back(cos(r)); d.push_back(0); d.push_back(sin(r)); d.push_back(0); d.push_back(1); d.push_back(0); }
    GLuint vbo; glGenVertexArrays(1, &orbitVAO); glGenBuffers(1, &vbo); glBindVertexArray(orbitVAO);
    glBindBuffer(GL_ARRAY_BUFFER, vbo); glBufferData(GL_ARRAY_BUFFER, d.size() * 4, &d[0], GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, 0, 24, 0); glEnableVertexAttribArray(0);
}

// Initializarea
void init() {
    glClearColor(0.005f, 0.005f, 0.015f, 1);
    glEnable(GL_DEPTH_TEST); // Obiectele din fata le acopera pe cele din spate
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_PROGRAM_POINT_SIZE);

    // Lista planetelor cu proprietatile lor
    sys = {
        {"Mercur", 10,0.8f,4.5f, 0.7f,0.7f,0.7f, 1, false, 0, "167 C", "0.33e24 kg"},
        {"Venus",  16,1.2f,3.5f, 0.9f,0.8f,0.5f, 1, false, 0, "464 C", "4.87e24 kg"},
        {"Pamant", 24,1.3f,2.8f, 0.0f,0.0f,0.0f, 4, false, 0, "15 C", "5.97e24 kg"},
        {"Marte",  32,1.0f,2.2f, 0.8f,0.3f,0.1f, 1, false, 0, "-65 C", "0.64e24 kg"},
        {"Jupiter",55,4.5f,1.2f, 0.8f,0.7f,0.6f, 2, false, 0, "-110 C", "1898e24 kg"},
        {"Saturn", 75,4.0f,0.9f, 0.9f,0.8f,0.5f, 2, true,  27, "-140 C", "568e24 kg"}, // Inclinat
        {"Uranus", 95,3.0f,0.6f, 0.4f,0.8f,0.9f, 3, true,  90, "-195 C", "86e24 kg"},  // Foarte inclinat
        {"Neptun", 115,2.9f,0.5f,0.1f,0.1f,0.7f, 3, false, 0, "-200 C", "102e24 kg"}
    };

    GLuint v = mkS(GL_VERTEX_SHADER, vsSrc), f = mkS(GL_FRAGMENT_SHADER, fsSrc);
    prog = glCreateProgram(); glAttachShader(prog, v); glAttachShader(prog, f); glLinkProgram(prog);

    mkSphere(); mkStars(); mkOrbit(); mkRing(); mkAst();
}

// Calculam noile pozitii in timp
void update(int) {
    if (!paused) {
        tm += 0.01f * speed;
        for (auto& p : sys) {
            float a = tm * p.spd;
            p.cx = cos(a) * p.dist;
            p.cz = sin(a) * p.dist;
            p.cy = 0;
        }
    }
    glutPostRedisplay(); glutTimerFunc(16, update, 0);
}

void txt(float x, float y, const char* s, void* f = GLUT_BITMAP_HELVETICA_12) {
    glRasterPos2f(x, y); for (const char* c = s; *c; c++) glutBitmapCharacter(f, *c);
}

// Deseneaza interfata
void gui() {
    glUseProgram(0); glDisable(GL_LIGHTING); glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity(); gluOrtho2D(0, wW, wH, 0);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();

    glColor3f(0.8f, 0.8f, 0.8f);
    txt(10, 20, "CONTROALE: Click=Select | Drag=Rotire | +/-=Viteza");

    // Panoul cu informatii daca am dat click pe ceva
    if (sel != -1) {
        Planet& p = sys[sel];
        glEnable(GL_BLEND); glColor4f(0, 0.1f, 0.2f, 0.85f); glRectf(wW - 260, 20, wW - 20, 180); glDisable(GL_BLEND);
        glColor3f(1, 0.8f, 0); txt(wW - 240, 50, p.name.c_str(), GLUT_BITMAP_HELVETICA_18);
        glColor3f(1, 1, 1);
        txt(wW - 240, 80, ("Temp: " + p.i1).c_str());
        txt(wW - 240, 100, ("Masa: " + p.i2).c_str());
        glColor3f(0.6f, 0.6f, 0.6f); txt(wW - 240, 150, "[Click pt inchidere]");
    }
    glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix(); glMatrixMode(GL_MODELVIEW);
    glEnable(GL_DEPTH_TEST);
}

// Calculam matricile o singura data ca sa coincida Click-ul cu Desenarea
void calcMatrices() {
    Vec3 cp = { 0,0,0 };
    if (sel == -1) cp = { camDist * sin(DEG2RAD(camAY)) * cos(DEG2RAD(camAX)), camDist * sin(DEG2RAD(camAX)), camDist * cos(DEG2RAD(camAY)) * cos(DEG2RAD(camAX)) };
    else cp = { sys[sel].cx, 8, sys[sel].cz + 18 };
    Vec3 tgt = (sel == -1) ? Vec3{ 0,0,0 } : Vec3{ sys[sel].cx,0,sys[sel].cz };

    matView = Mat4::LookAt(cp, tgt, { 0,1,0 });
    matProj = Mat4::Perspective(DEG2RAD(45.0f), (float)wW / wH, 1.0f, 2000.0f);
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(prog);

    calcMatrices();

    glUniformMatrix4fv(glGetUniformLocation(prog, "view"), 1, 0, matView.m);
    glUniformMatrix4fv(glGetUniformLocation(prog, "projection"), 1, 0, matProj.m);
    glUniform3f(glGetUniformLocation(prog, "lightPos"), 0, 0, 0);

    // Setam pozitia camerei pentru calculul reflexiei
    float camX = (sel == -1) ? camDist * sin(DEG2RAD(camAY)) * cos(DEG2RAD(camAX)) : sys[sel].cx;
    float camY = (sel == -1) ? camDist * sin(DEG2RAD(camAX)) : 8;
    float camZ = (sel == -1) ? camDist * cos(DEG2RAD(camAY)) * cos(DEG2RAD(camAX)) : sys[sel].cz + 18;
    glUniform3f(glGetUniformLocation(prog, "viewPos"), camX, camY, camZ);

    // 1. STELE
    glUniform1i(glGetUniformLocation(prog, "type"), 0); glUniform3f(glGetUniformLocation(prog, "baseColor"), 1, 1, 1);
    Mat4 m = Mat4::Identity(); glUniformMatrix4fv(glGetUniformLocation(prog, "model"), 1, 0, m.m);
    glBindVertexArray(starVAO); glPointSize(1.5f); glDrawArrays(GL_POINTS, 0, starCnt);

    // 2. CENTURA ASTEROIZI
    Mat4 am = Mat4::RotateY(tm * 0.2f);
    glUniformMatrix4fv(glGetUniformLocation(prog, "model"), 1, 0, am.m);
    glUniform3f(glGetUniformLocation(prog, "baseColor"), 0.55f, 0.5f, 0.45f);
    glBindVertexArray(astVAO); glPointSize(2.0f); glDrawArrays(GL_POINTS, 0, astCnt);

    // 3. SOARELE
    Mat4 sm = Mat4::Scale(6, 6, 6); glUniformMatrix4fv(glGetUniformLocation(prog, "model"), 1, 0, sm.m);
    glUniform3f(glGetUniformLocation(prog, "baseColor"), 1, 0.9f, 0.1f);
    glBindVertexArray(sphereVAO); glDrawElements(GL_TRIANGLES, sphereCnt, GL_UNSIGNED_INT, 0);

    // 4. PLANETE & INELE
    for (auto& p : sys) {
        // Orbita
        glUniform1i(glGetUniformLocation(prog, "type"), 0);
        Mat4 om = Mat4::Scale(p.dist, p.dist, p.dist); glUniformMatrix4fv(glGetUniformLocation(prog, "model"), 1, 0, om.m);
        glUniform3f(glGetUniformLocation(prog, "baseColor"), 0.2f, 0.2f, 0.2f); glBindVertexArray(orbitVAO); glDrawArrays(GL_LINE_LOOP, 0, 361);

        // Planeta
        glUniform1i(glGetUniformLocation(prog, "type"), p.type);
        Mat4 pm = Mat4::Translate(p.cx, p.cy, p.cz) * Mat4::RotateY(tm * 5) * Mat4::Scale(p.rad, p.rad, p.rad);
        glUniformMatrix4fv(glGetUniformLocation(prog, "model"), 1, 0, pm.m);
        glUniform3f(glGetUniformLocation(prog, "baseColor"), p.r, p.g, p.b);
        glBindVertexArray(sphereVAO); glDrawElements(GL_TRIANGLES, sphereCnt, GL_UNSIGNED_INT, 0);

        // Inele
        if (p.hasRings) {
            glUniform1i(glGetUniformLocation(prog, "type"), 0);
            Mat4 rm = Mat4::Translate(p.cx, p.cy, p.cz) * Mat4::RotateX(DEG2RAD(p.ringInc)) * Mat4::Scale(p.rad, p.rad, p.rad);
            glUniformMatrix4fv(glGetUniformLocation(prog, "model"), 1, 0, rm.m);
            glUniform3f(glGetUniformLocation(prog, "baseColor"), p.r * 0.8f, p.g * 0.8f, p.b * 0.8f);
            glBindVertexArray(ringVAO); glDrawArrays(GL_TRIANGLE_STRIP, 0, ringCnt);
        }
    }
    gui(); glutSwapBuffers();
}

// Verifica daca am dat click pe o planeta
int pick(int mx, int my) {
    calcMatrices();

    GLint vp[4]; glGetIntegerv(GL_VIEWPORT, vp);
    GLdouble mv[16], pr[16];
    for (int i = 0; i < 16; i++) { mv[i] = matView.m[i]; pr[i] = matProj.m[i]; }

    for (int i = 0; i < sys.size(); i++) {
        GLdouble x, y, z;
        gluProject(sys[i].cx, sys[i].cy, sys[i].cz, mv, pr, vp, &x, &y, &z);
        float d = sqrt(pow(mx - x, 2) + pow(my - (vp[3] - y), 2));
        if (d < 30) return i; // Am nimerit planeta
    }
    return -1;
}

void mouse(int b, int s, int x, int y) {
    if (b == 3) camDist -= 5; if (b == 4) camDist += 5; // Zoom
    if (b == 0 && s == 0) {
        int p = pick(x, y);
        if (p != -1) sel = p; else if (sel != -1) sel = -1; else { drag = true; lMX = x; lMY = y; }
    }
    if (b == 0 && s == 1) drag = false;
}

void motion(int x, int y) { if (drag && sel == -1) { camAY += (x - lMX) * 0.5f; camAX += (y - lMY) * 0.5f; lMX = x; lMY = y; glutPostRedisplay(); } }

void kb(unsigned char k, int, int) { if (k == 27) exit(0); if (k == ' ') paused = !paused; if (k == '+' || k == '=') speed += 0.2f; if (k == '-') speed -= 0.2f; }

void rs(int w, int h) { wW = w; wH = h; glViewport(0, 0, w, h); }

int main(int c, char** v) {
    glutInit(&c, v); glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH); glutInitWindowSize(wW, wH);
    glutInitContextVersion(3, 3); glutInitContextProfile(GLUT_COMPATIBILITY_PROFILE);
    glutCreateWindow("Sistem Solar Final");
    glewInit(); init();
    glutDisplayFunc(display); glutReshapeFunc(rs); glutMouseFunc(mouse); glutMotionFunc(motion); glutKeyboardFunc(kb); glutTimerFunc(0, update, 0);
    glutMainLoop(); return 0;
}