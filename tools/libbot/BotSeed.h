#ifndef CATCHCHALLENGER_BOTSEED_H
#define CATCHCHALLENGER_BOTSEED_H

// Deterministic-run switch for the bot.
//
// The bot seeds three independent RNGs from wall clock / hardware entropy
// (srand(time()) twice, plus std::mt19937 from std::random_device). A load
// benchmark that A/B-compares two builds needs the SAME decision sequence in
// both, otherwise the runs do different work and their CPU numbers are not
// comparable -- measured spread was 572..966 moves between two runs of the
// same build.
//
// botSeed() returns 0 when --seed was not given (keep the historical random
// behaviour) or the seed to use for every RNG in the process.
unsigned int botSeed();
void setBotSeed(unsigned int seed);

// How often the AI may pick a new action, per bot, in milliseconds (default
// 1000). A benchmark calibrating a fast box runs out of headroom once the bot
// count is capped: the remaining way to load the server is to make each bot act
// more often. Raise the server's DDOS/kickLimitMove to match, or the faster
// bots get kicked as flooders.
unsigned int botActionIntervalMs();
void setBotActionIntervalMs(unsigned int ms);

#endif
