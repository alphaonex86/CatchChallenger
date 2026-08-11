#ifndef TERRAINSHAPER_H
#define TERRAINSHAPER_H

//The single hook on the raw height/moisure noise: every terrain decision of this
//generator (tile, transition, mountain wall, vegetation, wild monsters) is taken
//from those two values at a world position, so overriding them is enough to
//reshape the world without touching any drawing code.
//
//The base class returns the noise UNTOUCHED, so the standalone terrain generator
//keeps its exact behaviour. A generator that knows more about the world than the
//noise does - ../map-procedural-generation/ knows where its cities are - derives
//from it and installs its own with setActive().
class TerrainShaper
{
public:
    TerrainShaper();
    virtual ~TerrainShaper();
    //x and y are world TILE coordinates, height and moisure the raw noise values
    virtual void shape(const float x,const float y,float &height,float &moisure) const;
    //the shaper every noise sample goes through, never NULL
    static TerrainShaper *active();
    //NULL restores the untouched-noise default
    static void setActive(TerrainShaper *shaper);
};

#endif // TERRAINSHAPER_H
