#pragma once
#include <scene/materials/bxdfs/bxdf.h>

class BlinnMicrofacetBxDF : public BxDF
{
public:
//Constructors/Destructors
    BlinnMicrofacetBxDF() : BlinnMicrofacetBxDF(glm::vec3(0.5f), 1.0f)
    {}
    BlinnMicrofacetBxDF(const glm::vec3 &color) : BlinnMicrofacetBxDF(color, 1.0f)
    {}
    BlinnMicrofacetBxDF(const glm::vec3 &color, float exp) : BxDF(BxDFType(BSDF_REFLECTION | BSDF_SPECULAR)), reflection_color(color), exponent(exp)
    {}
//Functions
    virtual glm::vec3 EvaluateScatteredEnergy(const glm::vec3 &wo, const glm::vec3 &wi) const;
    virtual glm::vec3 EvaluateHemisphereScatteredEnergy(const glm::vec3 &wo, int num_samples, const glm::vec2 *samples) const;
//Member variables
    glm::vec3 reflection_color;
    float exponent;
};
