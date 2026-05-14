/*
 * gj_model
 * Copyright (c) 2026 giji676
 * Licensed under MIT (see LICENSE file)
 */

#ifndef GJ_MODEL_H
#define GJ_MODEL_H

#ifdef __cplusplus
extern "C" {
#endif

struct gjMesh {
    char name[256];
    float *positions;
    float *normals;
    float *texcoords;

    unsigned int *indices;

    int vertexCount;
    int indexCount;

    int materialIndex;
};

struct gjMaterial {
    char name[256];
    float ambient[3];
    float diffuse[3];
    float specular[3];
    float specularW;
    float transparency;
    float transmissionFilter[3];
    float opticalDensity;
    int illumination;

    char ambientMap[256];
    char diffuseMap[256];
    char specularMap[256];
    char specularHighlightMap[256];
    char alphaMap[256];
    char bumpMap[256];
    char displacementMap[256];
    char stencilMap[256];
};

struct gjNode {
    float transform[16];

    int *meshIndices;
    int meshCount;

    struct gjNode *children;
    int childCount;
};

struct gjModel {
    struct gjNode root;

    struct gjMesh *meshes;
    int meshCount;

    struct gjMaterial *materials;
    int materialCount;
};

struct gjModel *malloc_model();
struct gjNode *malloc_node();
struct gjMesh *malloc_mesh();
struct gjMaterial *malloc_material();

/*
 * Opens a 3D model file
 *
 * @param filename Path to the model file
 *
 * @return A struct gjModel
 *
 * The returned struct gjModel must be freed using gj_model_free().
 */
struct gjModel *gj_model_load(const char *filename);

/*
 * Frees the allocated image data
 *
 * @param data Pointer to the model to be freed
 */
void            gj_model_free(struct gjModel *model);

#ifdef __cplusplus
}
#endif

#endif /* GJ_MODEL_H */
