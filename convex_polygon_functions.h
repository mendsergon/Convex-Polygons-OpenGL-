#pragma once

#include <GL/glut.h>
#include <AntTweakBar.h>
#include <vector> 
#include <iostream>
#include <algorithm> 
#include <cmath>
#include <random>
#include <ctime>

// Simple 2D point
struct  Point // Store x and y coordinates of each point of the polygon
{
    float x, y;
};

// Global variables for number of sides and the polygon's points
extern int currentNumSides;
extern std::vector<Point> convextPolygon;


// Function declarations
void generateConvexPolygon(int n);
float crossProduct(const Point& o, const Point& a, const Point& b);
void getBoundingBox(const std::vector<Point>& Polygon, float& minX, float& maxX, float& minY, float& maxY);
bool isInside(const Point& p, const Point& edgeStart, const Point& edgeEnd);
void fillPolygon(const std::vector<Point>& Polygon);
void display();
void onNumSidesChanged(int controlID);
void setupAntTweakBar();

// Andrew's monotone-chain convex hull
static std::vector<Point> computeHull(std::vector<Point> pts)
{
    if (pts.size() < 3) return pts;
    // Sort lexicographically
    std::sort(pts.begin(), pts.end(), [](auto &A, auto &B)
    {
        return (A.x < B.x) || (A.x == B.x && A.y < B.y);
    });
    std::vector<Point> H;

    // Lower hull
    for (auto &p : pts)
    {
        while (H.size() >= 2 && crossProduct(H[H.size() - 2], H.back(), p) <= 0)
            H.pop_back();
            H.push_back(p);
    }

    // Upper hull
    for (int i = (int)pts.size() - 2, t = H.size() + 1; i >= 0; --i)
    {
        auto &p = pts[i];
        while (H.size() >= t && crossProduct(H[H.size() - 2], H.back(), p) <= 0)
            H.pop_back();
            H.push_back(p);
    }
    H.pop_back();
    return H;
}

// Function to generate the polygon with 'n' sides
void generateConvexPolygon(int n) 
{
    if (n < 3)
    {
        std::cerr << "Error: A polygon requires at least 3 points." << std::endl;
        convextPolygon.clear();
        return;
    }

    // Generate n unique random points
    std::vector<Point> pts;
    pts.reserve(n);
    std::mt19937 rng((unsigned)std::time(nullptr));
    std::uniform_int_distribution<int> dx(20, 619), dy(20, 459);
    while ((int)pts.size() < n)
    {
        Point p{float(dx(rng)), float(dy(rng))};
        bool dup = false;
        for (auto &q : pts)
        {
            if (q.x == p.x && q.y == p.y)
            {
                dup = true;
                break;
            } 
        }
        if (!dup) pts.push_back(p);
    }

    // Compute true convex hull
    auto hull = computeHull(pts);
    int h = (int)hull.size();
    if (h == n)
    {
        convextPolygon = std::move(hull);
        return;
    }

    // Subdivide edges if h < n
    int need = n - h;
    std::vector<int> subdiv(h, need / h);
    for (int i = 0; i < need % h; ++i) subdiv[i]++;

    std::vector<Point> boundary;
    boundary.reserve(n);
    for (int i = 0; i < h; ++i)
    {
        Point &A = hull[i], &B =hull[(i + 1) % h];
        boundary.push_back(A);
        int k = subdiv[i];
        for (int j = 1; j <= k; ++j)
        {
            float t = float(j) / float(k + 1);
            boundary.push_back({ A.x * (1 - t) + B.x * t, A.y * (1 - t) + B.y * t});
        }
    }

    // CCW ordering around centroid
    Point centroid{0, 0};
    for (auto &p : boundary)
    {
        centroid.x += p.x; centroid.y += p.y;
    }
    centroid.x /= n;
    centroid.y /= n;
    std::sort(boundary.begin(), boundary.end(), [&](auto &A, auto &B)
    {
        return atan2(A.y - centroid.y, A.x - centroid.x) < atan2(B.y - centroid.y, B.x - centroid.x);
    });
    convextPolygon = std::move(boundary);
}

// Compute the cross product of the two vectors to check the turn direction
float crossProduct(const Point& o, const Point& a, const Point& b)
{
    return (a.x - o.x) * (b.y - o.y) - (a.y - o.y) * (b.x - o.x);
}

// Find the bounding box
void getBoundingBox(const std::vector<Point>& Polygon, float& minX, float& maxX, float& minY, float& maxY)
{
    minX = maxX = Polygon[0].x;
    minY = maxY = Polygon[0].y;

    for (const Point& p : Polygon)
    {
        if (p.x < minX) minX = p.x;
        if (p.x > maxX) maxX = p.x;
        if (p.y < minY) minY = p.y;
        if (p.y > maxY) maxY = p.y;
    }
}

// Determine if a pixel is inside the polygon or not
bool isInside(const Point& p, const Point& edgeStart, const Point& edgeEnd)
{
    const float epsilon = 0.01f; // small tolerance
    return (edgeEnd.x - edgeStart.x) * (p.y - edgeStart.y) - (edgeEnd.y - edgeStart.y) * (p.x - edgeStart.x) >= 0;
}

// Loop through each pixel within the bounding box of the convex polygon and check if it is inside the polygon
void fillPolygon(const std::vector<Point>& Polygon)
{
    // Get bounding box
    float minX, maxX, minY, maxY;
    getBoundingBox(Polygon, minX, maxX, minY, maxY);

    // Cast bounds to int for pixel stepping
    int iMinX = static_cast<int>(std::floor(minX));
    int iMaxX = static_cast<int>(std::ceil(maxX));
    int iMinY = static_cast<int>(std::floor(minY));
    int iMaxY = static_cast<int>(std::ceil(maxY));

    // Loop through each pixel
    for (int y = iMinY; y <= iMaxY; ++y)
    {
        for (int x = iMinX; x <= iMaxX; ++x)
        {
            // Sample at pixel center
            Point p = { x+ 0.5f, y + 0.5f};

            bool inside = true;

            for (size_t i = 0; i < Polygon.size(); ++i)
            {
                const Point& start = Polygon[i];
                const Point& end = Polygon[(i + 1) % Polygon.size()];

                if (!isInside(p, start, end))
                {
                    inside = false;
                    break;
                }
            }

            if (inside)
            {
                // Draw pixel
                glBegin(GL_POINTS);
                glVertex2f(p.x, p.y);
                glEnd();
            }
        }
    }
}

// Callback function for when the slider is changed
void onNumSidesChanged(int) 
{
    // Regenerate polygon with the new number of sides
    generateConvexPolygon(currentNumSides);
    // Ask GLUT to redraw
    glutPostRedisplay();
}

// Function to set up AntTweakBar controls
void setupAntTweakBar() 
{
    // Initiate AntTweakbar
    TwInit(TW_OPENGL, NULL);

    // tell AntTweakBar the current window size now:
    int w = glutGet(GLUT_WINDOW_WIDTH), h = glutGet(GLUT_WINDOW_HEIGHT);
    TwWindowSize(w, h);

    // Create a tweak bar
    TwBar* bar = TwNewBar("Settings");

    // Let window size changes propagate to AntTweakBar
    glutReshapeFunc([](int w, int h)
    {
        TwWindowSize(w, h);
        glViewport(0, 0, w, h);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluOrtho2D(0, 640, 0, 480);
        glMatrixMode(GL_MODELVIEW);
    });

    // Add slider for number of sides: 3-1000
    TwAddVarCB(bar, "Sides", TW_TYPE_INT32,
        // setter: update, regenerate polygon
        [](const void* value, void* /*clientData*/) 
        {
            currentNumSides = *static_cast<const int*>(value);
            generateConvexPolygon(currentNumSides);
            glutPostRedisplay();
        },
        // getter: report the current value
        [](void* value, void* /*clientData*/) 
        {
            *static_cast<int*>(value) = currentNumSides;
        },
        nullptr, "min=3 max=1000 step=1 help='Number of polygon sides (up to 1000)'");

    // Route GLUT events into AntTweakBar:
    glutKeyboardFunc   (reinterpret_cast<GLUTkeyboardfun>       (TwEventKeyboardGLUT));
    glutMouseFunc      (reinterpret_cast<GLUTmousebuttonfun>   (TwEventMouseButtonGLUT));  
    glutMotionFunc     (reinterpret_cast<GLUTmousemotionfun>   (TwEventMouseMotionGLUT));  
    glutPassiveMotionFunc(reinterpret_cast<GLUTmousemotionfun>  (TwEventMouseMotionGLUT));
    glutSpecialFunc    (reinterpret_cast<GLUTspecialfun>        (TwEventSpecialGLUT));

}

// Function to display the polygon
void display() 
{
    // Set background to white and clear the screen
    glClearColor(1, 1, 1, 0); 
    glClear(GL_COLOR_BUFFER_BIT); 

    // Set polygon color (red)
    glColor3f(1.0f, 0.8f, 0.8f); 
    glPointSize(1.5f);

    // Fill the polygon manually
    fillPolygon(convextPolygon);

    // Draw polygon outline (black)
    glColor3f(0.0f, 0.0f, 0.0f); // Black edges
    glLineWidth(1.5f);
    glBegin(GL_LINE_LOOP);
    for (const Point& p : convextPolygon)
    {
        glVertex2f(p.x, p.y);
    }
    glEnd();

    // Draw polygon vertices (blue dots)
    glColor3f(0.0f, 0.0f, 1.0f); // Blue points
    glPointSize(6.0);
    glBegin(GL_POINTS);
    for (const Point& p : convextPolygon)
    {
        glVertex2f(p.x, p.y);
    }
    glEnd();

    TwDraw();

    glFlush(); // Execute pending commands
}
