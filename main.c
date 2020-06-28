/*  
    A simple 2D ray casting demo. The mouse wheel controls how many rays are cast.

    Brandon Luk
*/

#include <GLFW/glfw3.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define MONITOR_SIZE_X 1920
#define MONITOR_SIZE_Y 1080
static const double monitor_widescreen_compensation = (double) MONITOR_SIZE_X / MONITOR_SIZE_Y;

#define PI 3.14159265359

#define CIRCLE_RADIUS 0.05  // Radius for a circle around the mouse cursor whose circumference is used as a reference to which points from the cursor are extended

double RAY_DENSITY = 180.0; // Number of rays to cast

typedef struct{
    double x, y;
} Point;

typedef struct{
    Point point1, point2;
} Line;

static const Line walls[] = {   {{ 0.65,  0.5}, { 0.7,   0.8}},
                                {{ 0.2,   0.3}, { 0.4,  -0.2}},
                                {{ 0.4,  -0.2}, { 0.05, -0.3}},
                                {{ 0.05, -0.3}, {-0.2,  -0.1}},
                                {{-0.2,  -0.1}, {-0.1,   0.2}},
                                {{-0.1,   0.2}, { 0.2,   0.3}},
                                {{-0.5,   0.5}, {-0.3,   0.3}},
                                {{-0.3,   0.5}, {-0.5,   0.3}},
                                {{-0.5,  -0.5}, {-0.2,  -0.5}}   };



/*  Convert points from the GLFW coordinate system (where the origin is the top-left)
    to the OpenGL coordinate system (where the origin is in the center of the window). */
Point normalizeMonitorCoordinates(double xpos, double ypos)
{
    Point norm;
    norm.x = (xpos - MONITOR_SIZE_X / 2.0) / (MONITOR_SIZE_X / 2.0);
    norm.y = (ypos - MONITOR_SIZE_Y / 2.0) / (MONITOR_SIZE_Y / 2.0);

    if(norm.x > 1.0)
        norm.x = 1.0;
    else if(norm.x < -1.0)
        norm.x = -1.0;

    if(norm.y > 1.0)
        norm.y = 1.0;
    else if(norm.y < -1.0)
        norm.y = -1.0;


    /* Flip the y-axis, since GLFW defines "down" as positive and OpenGL the opposite */
    norm.y *= -1;

    return norm;
}

/* Return the distance between two points */
double pointDistance(Point p1, Point p2)
{
    return sqrt(pow(p2.x - p1.x, 2) + pow(p2.y - p1.y, 2));
}

/*  Find the intersection between two lines. If an intersection exists, return 1 and put the point of intersection in the intersection variable.
    If no intersection exists, return 0. */
int findLineSegmentIntersection(Line l1, Line l2, Point* intersection)
{

    /* Truncate the names of variables for readibility */
    double x1 = l1.point1.x;
    double y1 = l1.point1.y;

    double x2 = l1.point2.x;
    double y2 = l1.point2.y;

    double x3 = l2.point1.x;
    double y3 = l2.point1.y;

    double x4 = l2.point2.x;
    double y4 = l2.point2.y;

    double t = ((x1 - x3) * (y3 - y4) - (y1 - y3) * (x3 - x4)) /
                ((x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4));

    double u = - ((x1 - x2) * (y1 - y3) - (y1 - y2) * (x1 - x3)) / 
                    ((x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4));


    /* If t and u are within [0, 1], then there exists an intersection between the lines */
    if( 0.0 <= t && t <= 1.0 && 0.0 <= u && u <= 1.0)
    {
         intersection->x = x1 + t * (x2 - x1);
         intersection->y = y1 + t * (y2 - y1);
         return 1;
    }

    return 0;
}

/*  Return a line that is an extension of the provided argument. The first point of the expanded line will be the same as the argument line.
    The second point will be arbitrarily large along the slope of the provided argument. */
Line expandLine(Line original)
{
    static const double ARBITRARY_OUT_OF_BOUNDS_COORD = INT16_MAX; // Some arbitarily large value that will surely be outside the bounds of the screen

    Line expanded;
    expanded.point1 = original.point1;

    /* Find the equation of the line */

    /* If the x's of the two points are the same the line is vertical */
    if(fabs(original.point2.x - original.point1.x) < 10e-7)
    {
        if(original.point2.y < original.point1.y)
        {
            expanded.point2 = (Point){original.point1.x, (-1) * ARBITRARY_OUT_OF_BOUNDS_COORD};
        }
        else
        {
            expanded.point2 = (Point){original.point1.x, ARBITRARY_OUT_OF_BOUNDS_COORD};
        }
        
        return expanded;
    }
    /* If the y's of the two points are the same the line is horizontal */
    else if(fabs(original.point2.y - original.point1.y) < 10e-7)
    {
        if(original.point2.x < original.point1.x)
        {
            expanded.point2 = (Point){(-1.0) * ARBITRARY_OUT_OF_BOUNDS_COORD, original.point1.y};
        }
        else
        {
            expanded.point2 = (Point){ARBITRARY_OUT_OF_BOUNDS_COORD, original.point1.y};
        }
        
        return expanded;
    }

    double slope = (original.point2.y - original.point1.y) / (original.point2.x - original.point1.x);
    double y_intercept = original.point1.y - (slope * original.point1.x);

    expanded.point2.x = ARBITRARY_OUT_OF_BOUNDS_COORD;
    expanded.point2.y = (slope * expanded.point2.x) + y_intercept; 

    if(original.point2.x < original.point1.x)
        expanded.point2.x *= -1;
    if(original.point2.y < original.point1.y)
        expanded.point2.y *= 1;

    return expanded;
}

void evaluateIntersection(Line* start, Line* expanded, const Line* toIntersect, Point* intersection, Point* temp, double* dist_to_intersection)
{
    if(findLineSegmentIntersection(*expanded, *toIntersect, temp))
        if(pointDistance(start->point1, *temp) < *dist_to_intersection)
        {
            *dist_to_intersection = pointDistance(start->point1, *temp);
            *intersection = *temp;
        } 
}

/* Return the nearest point of intersection between the provided line and any other objects (either a wall or border) */
Point findNearestIntersectionPoint(Line start)
{
    static const Line borders[] = { {{-1.1,  1.1 }, { 1.1,  1.1}},  // North border
                                    {{ 1.1,  1.1 }, { 1.1, -1.1}},  // East border
                                    {{-1.1, -1.1 }, { 1.1, -1.1}},  // South border
                                    {{-1.1,  1.1 }, {-1.1, -1.1}}}; // West border

    Point intersection;
    Point temp;
    double dist_to_intersection = (double) INT64_MAX; // An arbitrarily large value to begin with
    Line expanded = expandLine(start);

    /* Check if the line intersects any walls */
    for(int i = 0; i < (sizeof(walls) / sizeof(walls[0])); ++i)
    {
        evaluateIntersection(&start, &expanded, &walls[i], &intersection, &temp, &dist_to_intersection);
    }

    /* Check if the line intersects any borders */
    for(int i = 0; i < (sizeof(borders) / sizeof(borders[0])); ++i)
    {
        evaluateIntersection(&start, &expanded, &borders[i], &intersection, &temp, &dist_to_intersection);
    }

    return intersection;
}

void drawWall(const Line* w)
{
    glColor3f(1.0f, 1.0f, 1.0f);
    glLineWidth(5.0f);
    glEnable(GL_LINE_SMOOTH);

    glBegin(GL_LINES);
    glVertex2f(w->point1.x, w->point1.y);
    glVertex2f(w->point2.x, w->point2.y);


    glEnd();
}

void drawRays(GLFWwindow** window)
{
    /* Get the current position of the cursos to be used as the origin for the light */
    double xorigin, yorigin;
    glfwGetCursorPos(*window, &xorigin, &yorigin);
    
    Point normalizedOrigin = normalizeMonitorCoordinates(xorigin, yorigin);
    
    double inc = 2.0 * PI / RAY_DENSITY;

    Point onCircumference, endPoint;
    double xpos, ypos;

    glLineWidth(1.0f);

    glBegin(GL_LINES);
    for(int i = 0; i < (int) RAY_DENSITY; ++i)
    {
        xpos = normalizedOrigin.x + CIRCLE_RADIUS * cos(i * inc);
        ypos = normalizedOrigin.y + (CIRCLE_RADIUS * monitor_widescreen_compensation) * sin(i * inc);
        onCircumference = (Point){xpos, ypos};

        endPoint = findNearestIntersectionPoint((Line){normalizedOrigin, onCircumference});

        glVertex2d(normalizedOrigin.x, normalizedOrigin.y);
        glVertex2d(endPoint.x, endPoint.y);
    }
    glEnd();
}

/* Scrolling the mousewheel down reduces the number of rays projected. Scrolling up increases the number. */
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    RAY_DENSITY += yoffset;
    
    if(RAY_DENSITY < 0.0)
        RAY_DENSITY = 0.0;
    else  if(RAY_DENSITY > 1080.0) // An arbitrary limit. Can go higher, but begins to make your eyes hurt.
        RAY_DENSITY = 1080.0;
}

void initializeWindow(GLFWwindow** window)
{
    /* Set the window to be non-resizable by the user */
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    /* Create a windowed mode window and its OpenGL context */
    *window = glfwCreateWindow(1920, 1080, "Raycaster", NULL, NULL);
    if (!(*window))
    {
        glfwTerminate();
        exit(-1);
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(*window);

    /* Turn on scrolling input */
    glfwSetScrollCallback(*window, scroll_callback);
}

int main(void)
{
    GLFWwindow* window;

    /* Initialize the library */
    if (!glfwInit())
        return -1;

    initializeWindow(&window);

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {
        /* Render here */
        glClear(GL_COLOR_BUFFER_BIT);
        drawRays(&window);

        // Draw walls
        for(int i = 0; i < (sizeof(walls) / sizeof(walls[0])); ++i)
        {
            drawWall(&walls[i]);
        }

        /* Swap front and back buffers */
        glfwSwapBuffers(window);

        /* Poll for and process events */
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}