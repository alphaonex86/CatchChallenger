#include "BotSeed.h"

static unsigned int botSeedValue=0;

unsigned int botSeed()
{
    return botSeedValue;
}

void setBotSeed(unsigned int seed)
{
    botSeedValue=seed;
}

static unsigned int botActionIntervalValue=1000;

unsigned int botActionIntervalMs()
{
    return botActionIntervalValue;
}

void setBotActionIntervalMs(unsigned int ms)
{
    if(ms>0)
        botActionIntervalValue=ms;
}
