#pragma once
#include <QList>
#include <raytracing/film.h>
#include <scene/camera.h>
#include <raytracing/samplers/pixelsampler.h>
#include <scene/geometry/geometry.h>
#include <scene/materials/bxdfs/bxdf.h>
#include <scene/boundingbox.h>
#include <scene/geometry/square.h>

class Geometry;
class Material;
class SquarePlane;

class Scene
{
public:
    Scene();
    QList<Geometry*> objects;
    QList<Material*> materials;
    QList<BxDF*> bxdfs;
    QList<Geometry*> lights;
    Camera camera;
    Film film;

    unsigned int sqrt_samples;//Read by MyGL and RenderThread when making PixelSamplers

    //---all bounding boxes to be drawn---
    static QList< BoundingBox * > allBBoxes;

    //---square plane for progressive rendering---
    SquarePlane* canvas;

    void SetCamera(const Camera &c);

    void CreateTestScene();
    void Clear();
};
