
#include "directlightingintegrator.h"
#include "bidirectionalpathtracinghelper.h"

BidirectionalPathTracingHelper::BidirectionalPathTracingHelper():
    BidirectionalPathTracingHelper( NULL, NULL ){
}

BidirectionalPathTracingHelper::BidirectionalPathTracingHelper(
        Scene *scene,
        IntersectionEngine *intersection_engine ){

    this->scene = scene;
    this->intersection_engine = intersection_engine;
}

void BidirectionalPathTracingHelper::generatePath(
        vector< Intersection > &path_vertices,
        vector< glm::vec3 > &path_weights,
        Geometry *pLight,
        int max_depth ){

    //---initialization---
    path_vertices.clear();
    path_weights.clear();

    int depth( 0 );
    float PDF( 0.f );
    glm::vec3 wiW( 0.f );

    //---initial vertex on the light---
    float rand1( distribution( generator ) );
    float rand2( distribution( generator ) );
    float rand3( distribution( generator ) );
    Intersection isx( pLight->SampleLight( rand1, rand2, rand3 ) );

    //---generate path vertices---
    while( depth < max_depth ){

        //---sample direction from current vertex---
        isx.object_hit->material->SampleAndEvaluateScatteredEnergy( isx, glm::vec3( 0.f ), wiW, PDF );

        //---shoot ray to find next vertex---
        Ray ray( isx.point + isx.normal * 1e-4f, wiW );
        Intersection isx_new( intersection_engine->GetIntersection( ray ) );
        glm::vec3 wiW_new( 0.f );
        float pdf_bxdf( 0.f );

        if( isx_new.object_hit == NULL ){
            //---hit nothing---
            break;
        }else if( isx_new.object_hit->material->is_light_source ){
            //---hit light---
            break;
        }

        //---hit object---
        path_vertices.push_back( isx_new );

        if( depth == 0 ){
            //---first vertex, direct lighting only---
            glm::vec3 bxdf( isx_new.object_hit->material->SampleAndEvaluateScatteredEnergy( isx_new, -wiW, wiW_new, pdf_bxdf ) );
            glm::vec3 Ld( pLight->material->EvaluateScatteredEnergy( isx, glm::vec3( 0.f ), wiW ) );
            float absdot( fabsf( glm::dot( isx.normal, -wiW ) ) );
            float pdf_light( pLight->RayPDF( isx, Ray( isx_new.point, isx.point - isx_new.point ) ) );

            path_weights.push_back( bxdf * Ld * absdot / pdf_light );

        }else{
            //---not first vertex, balanced direct and indirect lighting---
            glm::vec3 bxdf( isx_new.object_hit->material->SampleAndEvaluateScatteredEnergy( isx_new, -wiW, wiW_new, pdf_bxdf ) );
            glm::vec3 Li( path_weights[ path_weights.size() - 2 ] );
            float absdot( fabsf( glm::dot( isx.normal, -wiW ) ) );

            path_weights.push_back( bxdf * Li * absdot );
        }

        //---iterate---
        isx = isx_new;
        ++depth;
    }
}
