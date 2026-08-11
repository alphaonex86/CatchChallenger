#include "TerrainShaper.h"

#include <cstddef>

//the untouched-noise shaper, used until someone installs another one. Taking its
//address is a constant expression, so no static initialisation order problem.
static TerrainShaper terrainShaperDefault;
static TerrainShaper *terrainShaperActive=&terrainShaperDefault;

TerrainShaper::TerrainShaper()
{
}

TerrainShaper::~TerrainShaper()
{
}

void TerrainShaper::shape(const float x,const float y,float &height,float &moisure) const
{
    (void)x;
    (void)y;
    (void)height;
    (void)moisure;
}

TerrainShaper *TerrainShaper::active()
{
    return terrainShaperActive;
}

void TerrainShaper::setActive(TerrainShaper *shaper)
{
    if(shaper==NULL)
        terrainShaperActive=&terrainShaperDefault;
    else
        terrainShaperActive=shaper;
}
