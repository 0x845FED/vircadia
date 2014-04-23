//
//  Util.cpp
//  interface/src
//
//  Created by Philip Rosedale on 8/24/12.
//  Copyright 2012 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <iostream>
#include <cstring>
#include <time.h>
#include <math.h>

#include <glm/glm.hpp>
#include <glm/gtc/noise.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/detail/func_common.hpp>

#include <SharedUtil.h>

#include "InterfaceConfig.h"
#include "ui/TextRenderer.h"
#include "VoxelConstants.h"
#include "world.h"

#include "Util.h"

using namespace std;

// no clue which versions are affected...
#define WORKAROUND_BROKEN_GLUT_STROKES
// see http://www.opengl.org/resources/libraries/glut/spec3/node78.html

void eulerToOrthonormals(glm::vec3 * angles, glm::vec3 * front, glm::vec3 * right, glm::vec3 * up) {
    //
    //  Converts from three euler angles to the associated orthonormal vectors
    //
    //  Angles contains (pitch, yaw, roll) in radians
    //

    //  First, create the quaternion associated with these euler angles
    glm::quat q(glm::vec3(angles->x, -(angles->y), angles->z));

    //  Next, create a rotation matrix from that quaternion
    glm::mat4 rotation;
    rotation = glm::mat4_cast(q);

    //  Transform the original vectors by the rotation matrix to get the new vectors
    glm::vec4 qup(0,1,0,0);
    glm::vec4 qright(-1,0,0,0);
    glm::vec4 qfront(0,0,1,0);
    glm::vec4 upNew    = qup*rotation;
    glm::vec4 rightNew = qright*rotation;
    glm::vec4 frontNew = qfront*rotation;

    //  Copy the answers to output vectors
    up->x = upNew.x;  up->y = upNew.y;  up->z = upNew.z;
    right->x = rightNew.x;  right->y = rightNew.y;  right->z = rightNew.z;
    front->x = frontNew.x;  front->y = frontNew.y;  front->z = frontNew.z;
}

void printVector(glm::vec3 vec) {
    qDebug("%4.2f, %4.2f, %4.2f", vec.x, vec.y, vec.z);
}

//  Return the azimuth angle (in radians) between two points.
float azimuth_to(glm::vec3 head_pos, glm::vec3 source_pos) {
    return atan2(head_pos.x - source_pos.x, head_pos.z - source_pos.z);
}

// Return the angle (in radians) between the head and an object in the scene.  
// The value is zero if you are looking right at it.
// The angle is negative if the object is to your right.
float angle_to(glm::vec3 head_pos, glm::vec3 source_pos, float render_yaw, float head_yaw) {
    return atan2(head_pos.x - source_pos.x, head_pos.z - source_pos.z) + render_yaw + head_yaw;
}

//  Draw a 3D vector floating in space
void drawVector(glm::vec3 * vector) {
    glDisable(GL_LIGHTING);
    glEnable(GL_POINT_SMOOTH);
    glPointSize(3.0);
    glLineWidth(2.0);

    //  Draw axes
    glBegin(GL_LINES);
    glColor3f(1,0,0);
    glVertex3f(0,0,0);
    glVertex3f(1,0,0);
    glColor3f(0,1,0);
    glVertex3f(0,0,0);
    glVertex3f(0, 1, 0);
    glColor3f(0,0,1);
    glVertex3f(0,0,0);
    glVertex3f(0, 0, 1);
    glEnd();

    // Draw the vector itself
    glBegin(GL_LINES);
    glColor3f(1,1,1);
    glVertex3f(0,0,0);
    glVertex3f(vector->x, vector->y, vector->z);
    glEnd();

    // Draw spheres for magnitude
    glPushMatrix();
    glColor3f(1,0,0);
    glTranslatef(vector->x, 0, 0);
    glutSolidSphere(0.02, 10, 10);
    glColor3f(0,1,0);
    glTranslatef(-vector->x, vector->y, 0);
    glutSolidSphere(0.02, 10, 10);
    glColor3f(0,0,1);
    glTranslatef(0, -vector->y, vector->z);
    glutSolidSphere(0.02, 10, 10);
    glPopMatrix();

}

void renderWorldBox() {
    //  Show edge of world
    float red[] = {1, 0, 0};
    float green[] = {0, 1, 0};
    float blue[] = {0, 0, 1};
    float gray[] = {0.5, 0.5, 0.5};

    glDisable(GL_LIGHTING);
    glLineWidth(1.0);
    glBegin(GL_LINES);
    glColor3fv(red);
    glVertex3f(0, 0, 0);
    glVertex3f(TREE_SCALE, 0, 0);
    glColor3fv(green);
    glVertex3f(0, 0, 0);
    glVertex3f(0, TREE_SCALE, 0);
    glColor3fv(blue);
    glVertex3f(0, 0, 0);
    glVertex3f(0, 0, TREE_SCALE);
    glColor3fv(gray);
    glVertex3f(0, 0, TREE_SCALE);
    glVertex3f(TREE_SCALE, 0, TREE_SCALE);
    glVertex3f(TREE_SCALE, 0, TREE_SCALE);
    glVertex3f(TREE_SCALE, 0, 0);
    glEnd();
    //  Draw meter markers along the 3 axis to help with measuring things
    const float MARKER_DISTANCE = 1.f;
    const float MARKER_RADIUS = 0.05f;
    glEnable(GL_LIGHTING);
    glPushMatrix();
    glTranslatef(MARKER_DISTANCE, 0, 0);
    glColor3fv(red);
    glutSolidSphere(MARKER_RADIUS, 10, 10);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(0, MARKER_DISTANCE, 0);
    glColor3fv(green);
    glutSolidSphere(MARKER_RADIUS, 10, 10);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(0, 0, MARKER_DISTANCE);
    glColor3fv(blue);
    glutSolidSphere(MARKER_RADIUS, 10, 10);
    glPopMatrix();
    glPushMatrix();
    glColor3fv(gray);
    glTranslatef(MARKER_DISTANCE, 0, MARKER_DISTANCE);
    glutSolidSphere(MARKER_RADIUS, 10, 10);
    glPopMatrix();

}

double diffclock(timeval *clock1,timeval *clock2)
{
	double diffms = (clock2->tv_sec - clock1->tv_sec) * 1000.0;
    diffms += (clock2->tv_usec - clock1->tv_usec) / 1000.0;   // us to ms

	return diffms;
}

//  Return a random vector of average length 1
const glm::vec3 randVector() {
    return glm::vec3(randFloat() - 0.5f, randFloat() - 0.5f, randFloat() - 0.5f) * 2.f;
}

static TextRenderer* textRenderer(int mono) {
    static TextRenderer* monoRenderer = new TextRenderer(MONO_FONT_FAMILY); 
    static TextRenderer* proportionalRenderer = new TextRenderer(SANS_FONT_FAMILY, -1, -1, false, TextRenderer::SHADOW_EFFECT); 
    static TextRenderer* inconsolataRenderer = new TextRenderer(INCONSOLATA_FONT_FAMILY, -1, QFont::Bold, false);
    switch (mono) {
        case 1:
            return monoRenderer;
        case 2:
            return inconsolataRenderer;
        case 0:
        default:
            return proportionalRenderer;
    }
}

int widthText(float scale, int mono, char const* string) {
    return textRenderer(mono)->computeWidth(string) * (scale / 0.10);
}

float widthChar(float scale, int mono, char ch) {
    return textRenderer(mono)->computeWidth(ch) * (scale / 0.10);
}

void drawText(int x, int y, float scale, float radians, int mono,
              char const* string, const float* color) {
    //
    //  Draws text on screen as stroked so it can be resized
    //
    glPushMatrix();
    glTranslatef(static_cast<float>(x), static_cast<float>(y), 0.0f);
    glColor3fv(color);
    glRotated(double(radians * DEGREES_PER_RADIAN), 0.0, 0.0, 1.0);
    glScalef(scale / 0.1f, scale / 0.1f, 1.f);
    textRenderer(mono)->draw(0, 0, string);
    glPopMatrix();
}


void drawvec3(int x, int y, float scale, float radians, float thick, int mono, glm::vec3 vec, float r, float g, float b) {
    //
    //  Draws vec3 on screen as stroked so it can be resized
    //
    char vectext[20];
    sprintf(vectext,"%3.1f,%3.1f,%3.1f", vec.x, vec.y, vec.z);
    int len, i;
    glPushMatrix();
    glTranslatef(static_cast<float>(x), static_cast<float>(y), 0);
    glColor3f(r,g,b);
    glRotated(180.0 + double(radians * DEGREES_PER_RADIAN), 0.0, 0.0, 1.0);
    glRotated(180.0, 0.0, 1.0, 0.0);
    glLineWidth(thick);
    glScalef(scale, scale, 1.f);
    len = (int) strlen(vectext);
	for (i = 0; i < len; i++) {
        if (!mono) glutStrokeCharacter(GLUT_STROKE_ROMAN, int(vectext[i]));
        else glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN, int(vectext[i]));
	}
    glPopMatrix();
}

void renderCollisionOverlay(int width, int height, float magnitude, float red, float blue, float green) {
    const float MIN_VISIBLE_COLLISION = 0.01f;
    if (magnitude > MIN_VISIBLE_COLLISION) {
        glColor4f(red, blue, green, magnitude);
        glBegin(GL_QUADS);
        glVertex2f(0, 0);
        glVertex2d(width, 0);
        glVertex2d(width, height);
        glVertex2d(0, height);
        glEnd();
    }
}

void renderSphereOutline(glm::vec3 position, float radius, int numSides, glm::vec3 cameraPosition) {
    glm::vec3 vectorToPosition(glm::normalize(position - cameraPosition));
    glm::vec3 right = glm::cross(vectorToPosition, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::vec3 up    = glm::cross(right, vectorToPosition);

    glBegin(GL_LINE_STRIP);
    for (int i=0; i<numSides+1; i++) {
        float r = ((float)i / (float)numSides) * TWO_PI;
        float s = radius * sinf(r);
        float c = radius * cosf(r);

        glVertex3f
        (
            position.x + right.x * s + up.x * c,
            position.y + right.y * s + up.y * c,
            position.z + right.z * s + up.z * c
        );
    }

    glEnd();
}


void renderCircle(glm::vec3 position, float radius, glm::vec3 surfaceNormal, int numSides) {
    glm::vec3 perp1 = glm::vec3(surfaceNormal.y, surfaceNormal.z, surfaceNormal.x);
    glm::vec3 perp2 = glm::vec3(surfaceNormal.z, surfaceNormal.x, surfaceNormal.y);

    glBegin(GL_LINE_STRIP);

    for (int i=0; i<numSides+1; i++) {
        float r = ((float)i / (float)numSides) * TWO_PI;
        float s = radius * sinf(r);
        float c = radius * cosf(r);
        glVertex3f
        (
            position.x + perp1.x * s + perp2.x * c,
            position.y + perp1.y * s + perp2.y * c,
            position.z + perp1.z * s + perp2.z * c
        );
    }
    glEnd();
}


void renderBevelCornersRect(int x, int y, int width, int height, int bevelDistance) {
    glBegin(GL_POLYGON);
    
    // left side
    glVertex2f(x, y + bevelDistance);
    glVertex2f(x, y + height - bevelDistance);
    
    // top side
    glVertex2f(x + bevelDistance,  y + height);
    glVertex2f(x + width - bevelDistance, y + height);
    
    // right
    glVertex2f(x + width, y + height - bevelDistance);
    glVertex2f(x + width, y + bevelDistance);
    
    // bottom
    glVertex2f(x + width - bevelDistance,  y);
    glVertex2f(x +bevelDistance, y);

    glEnd();
}

void renderRoundedCornersRect(int x, int y, int width, int height, int radius, int numPointsCorner) {
#define MAX_POINTS_CORNER 50
    // At least "2" is needed
    if (numPointsCorner <= 1) {
        return;
    }
    if (numPointsCorner > MAX_POINTS_CORNER) {
        numPointsCorner = MAX_POINTS_CORNER;
    }

    // Precompute sin and cos for [0, PI/2) for the number of points (numPointCorner)
    double radiusTimesSin[MAX_POINTS_CORNER];
    double radiusTimesCos[MAX_POINTS_CORNER];
    int i = 0;
    for (int i = 0; i < numPointsCorner; i++) {
        double t = (double)i * (double)PI_OVER_TWO / (double)(numPointsCorner - 1); 
        radiusTimesSin[i] = radius * sin(t);
        radiusTimesCos[i] = radius * cos(t);
    }

    glm::dvec2 cornerCenter;
    glBegin(GL_POINTS);
   
    // Top left corner
    cornerCenter = glm::vec2(x + radius, y + height - radius);
    for (i = 0; i < numPointsCorner; i++) {
        glVertex2d(cornerCenter.x - radiusTimesCos[i], cornerCenter.y + radiusTimesSin[i]); 
    }

    // Top rigth corner
    cornerCenter = glm::vec2(x + width - radius, y + height - radius);
    for (i = 0; i < numPointsCorner; i++) {
        glVertex2d(cornerCenter.x + radiusTimesSin[i], cornerCenter.y + radiusTimesCos[i]); 
    }

    // Bottom right
    cornerCenter = glm::vec2(x + width - radius, y + radius);
    for (i = 0; i < numPointsCorner; i++) {
        glVertex2d(cornerCenter.x + radiusTimesCos[i], cornerCenter.y - radiusTimesSin[i]); 
    }

    // Bottom left
    cornerCenter = glm::vec2(x + radius, y + radius);
    for (i = 0; i < numPointsCorner; i++) {
        glVertex2d(cornerCenter.x - radiusTimesSin[i], cornerCenter.y - radiusTimesCos[i]); 
    }
    glEnd();
}


void renderOrientationDirections(glm::vec3 position, const glm::quat& orientation, float size) {
	glm::vec3 pRight	= position + orientation * IDENTITY_RIGHT * size;
	glm::vec3 pUp		= position + orientation * IDENTITY_UP    * size;
	glm::vec3 pFront	= position + orientation * IDENTITY_FRONT * size;

	glColor3f(1.0f, 0.0f, 0.0f);
	glBegin(GL_LINE_STRIP);
	glVertex3f(position.x, position.y, position.z);
	glVertex3f(pRight.x, pRight.y, pRight.z);
	glEnd();

	glColor3f(0.0f, 1.0f, 0.0f);
	glBegin(GL_LINE_STRIP);
	glVertex3f(position.x, position.y, position.z);
	glVertex3f(pUp.x, pUp.y, pUp.z);
	glEnd();

	glColor3f(0.0f, 0.0f, 1.0f);
	glBegin(GL_LINE_STRIP);
	glVertex3f(position.x, position.y, position.z);
	glVertex3f(pFront.x, pFront.y, pFront.z);
	glEnd();
}

bool closeEnoughForGovernmentWork(float a, float b) {
    float distance = std::abs(a-b);
    //qDebug("closeEnoughForGovernmentWork() a=%1.10f b=%1.10f distance=%1.10f\n",a,b,distance);
    return (distance < 0.00001f);
}

//  Do some basic timing tests and report the results
void runTimingTests() {
    //  How long does it take to make a call to get the time?
    const int numTests = 1000000;
    int iResults[numTests];
    float fTest = 1.0;
    float fResults[numTests];
    timeval startTime, endTime;
    float elapsedMsecs;
    gettimeofday(&startTime, NULL);
    for (int i = 1; i < numTests; i++) {
        gettimeofday(&endTime, NULL);
    }
    elapsedMsecs = diffclock(&startTime, &endTime);
    qDebug("gettimeofday() usecs: %f", 1000.0f * elapsedMsecs / (float) numTests);
    
    // Random number generation
    gettimeofday(&startTime, NULL);
    for (int i = 1; i < numTests; i++) {
        iResults[i] = rand();
    }
    gettimeofday(&endTime, NULL);
    elapsedMsecs = diffclock(&startTime, &endTime);
    qDebug("rand() stored in array usecs: %f, first result:%d", 1000.0f * elapsedMsecs / (float) numTests, iResults[0]);

    // Random number generation using randFloat()
    gettimeofday(&startTime, NULL);
    for (int i = 1; i < numTests; i++) {
        fResults[i] = randFloat();
    }
    gettimeofday(&endTime, NULL);
    elapsedMsecs = diffclock(&startTime, &endTime);
    qDebug("randFloat() stored in array usecs: %f, first result: %f", 1000.0f * elapsedMsecs / (float) numTests, fResults[0]);

    //  PowF function
    fTest = 1145323.2342f;
    gettimeofday(&startTime, NULL);
    for (int i = 1; i < numTests; i++) {
        fTest = powf(fTest, 0.5f);
    }
    gettimeofday(&endTime, NULL);
    elapsedMsecs = diffclock(&startTime, &endTime);
    qDebug("powf(f, 0.5) usecs: %f", 1000.0f * elapsedMsecs / (float) numTests);

    //  Vector Math
    float distance;
    glm::vec3 pointA(randVector()), pointB(randVector());
    gettimeofday(&startTime, NULL);
    for (int i = 1; i < numTests; i++) {
        //glm::vec3 temp = pointA - pointB;
        //float distanceSquared = glm::dot(temp, temp);
        distance = glm::distance(pointA, pointB);
    }
    gettimeofday(&endTime, NULL);
    elapsedMsecs = diffclock(&startTime, &endTime);
    qDebug("vector math usecs: %f [%f msecs total for %d tests], last result:%f",
             1000.0f * elapsedMsecs / (float) numTests, elapsedMsecs, numTests, distance);

    //  Vec3 test
    glm::vec3 vecA(randVector()), vecB(randVector());
    float result;

    gettimeofday(&startTime, NULL);
    for (int i = 1; i < numTests; i++) {
        glm::vec3 temp = vecA-vecB;
        result = glm::dot(temp,temp);
    }
    gettimeofday(&endTime, NULL);
    elapsedMsecs = diffclock(&startTime, &endTime);
    qDebug("vec3 assign and dot() usecs: %f, last result:%f", 1000.0f * elapsedMsecs / (float) numTests, result);
}

float loadSetting(QSettings* settings, const char* name, float defaultValue) {
    float value = settings->value(name, defaultValue).toFloat();
    if (glm::isnan(value)) {
        value = defaultValue;
    }
    return value;
}

bool rayIntersectsSphere(const glm::vec3& rayStarting, const glm::vec3& rayNormalizedDirection,
        const glm::vec3& sphereCenter, float sphereRadius, float& distance) {
    glm::vec3 relativeOrigin = rayStarting - sphereCenter;

    // compute the b, c terms of the quadratic equation (a is dot(direction, direction), which is one)
    float b = 2.0f * glm::dot(rayNormalizedDirection, relativeOrigin);
    float c = glm::dot(relativeOrigin, relativeOrigin) - sphereRadius * sphereRadius;

    // compute the radicand of the quadratic.  if less than zero, there's no intersection
    float radicand = b * b - 4.0f * c;
    if (radicand < 0.0f) {
        return false;
    }

    // compute the first solution of the quadratic
    float root = sqrtf(radicand);
    float firstSolution = -b - root;
    if (firstSolution > 0.0f) {
        distance = firstSolution / 2.0f;
        return true; // origin is outside the sphere
    }

    // now try the second solution
    float secondSolution = -b + root;
    if (secondSolution > 0.0f) {
        distance = 0.0f;
        return true; // origin is inside the sphere
    }

    return false;
}

bool pointInSphere(glm::vec3& point, glm::vec3& sphereCenter, double sphereRadius) {
    glm::vec3 diff = point - sphereCenter;
    double mag = sqrt(glm::dot(diff, diff));
    if (mag <= sphereRadius) {
        return true;
    }
    return false;
}
