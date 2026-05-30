#include <cstdio>
#include <math.h>
#include <stdlib.h>
#include <vector>
#include <algorithm>
#include <random>

#include "perlin.h"

typedef struct  {
    float x;
    float y;
} Vector2;

static float dot(const Vector2 v1, const Vector2 v2) {
    return v1.x * v2.x + v1.y * v2.y;
}

void shuffle(int arr[], int len) {
    for (int i = len - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int temp = arr[i];
        arr[i] = arr[j];
        arr[j] = temp;
    }
}

std::vector<int> makePermutation() {
    constexpr int len = 256;

    std::vector<int> p(len * 2);

    // fill 0..255
    for (int i = 0; i < len; i++) {
        p[i] = i;
    }

    // shuffle first half
    static std::random_device rd;
    static std::mt19937 rng(rd());

    std::shuffle(p.begin(), p.begin() + len, rng);

    // duplicate
    std::copy(p.begin(), p.begin() + len, p.begin() + len);

    return p;
}

std::vector<int> permutation;

static void initPermutation(void) {
    if (!permutation.empty()) return;
    permutation = makePermutation();
}

static Vector2 getConstantVector(int v) {
	// v is the value from the permutation table
	int h = v & 3;
    switch (h) {
        case 0: return (Vector2){1.0, 1.0};
        case 1: return (Vector2){-1.0, 1.0};
        case 2: return (Vector2){-1.0, -1.0};
        default: return (Vector2){1.0, -1.0};
    }
}

static float fade(float t) {
	return ((6.f * t - 15.f) * t + 10.f) * t * t * t;
}

static float lerp(float t, float a1, float a2) {
	return a1 + t * (a2 - a1);
}

float noise2D(float x, float y) {
    initPermutation();

    int X = ((int)floorf(x)) & 255;
    int Y = ((int)floorf(y)) & 255;

    float xf = x - floorf(x);
    float yf = y - floorf(y);

    Vector2 topRight    = { xf - 1.0f, yf - 1.0f };
    Vector2 topLeft     = { xf,        yf - 1.0f };
    Vector2 bottomRight = { xf - 1.0f, yf };
    Vector2 bottomLeft  = { xf,        yf };
	
	// Select a value from the permutation array for each of the 4 corners
    int valueTopRight = permutation[permutation[X + 1] + Y + 1];
    int valueTopLeft = permutation[permutation[X] + Y + 1];
    int valueBottomRight = permutation[permutation[X + 1] + Y];
    int valueBottomLeft = permutation[permutation[X] + Y];
	
    float dotTopRight = dot(topRight, getConstantVector(valueTopRight));
    float dotTopLeft = dot(topLeft, getConstantVector(valueTopLeft));
    float dotBottomRight = dot(bottomRight, getConstantVector(valueBottomRight));
    float dotBottomLeft = dot(bottomLeft, getConstantVector(valueBottomLeft));
	
    float u = fade(xf);
    float v = fade(yf);
	
    return lerp(
        u,
        lerp(v, dotBottomLeft, dotTopLeft),
        lerp(v, dotBottomRight, dotTopRight)
    );
}
