#ifndef CUSTOMRAND_H
#define CUSTOMRAND_H

#include <cstdint>

//The random source of the two generators, in place of rand()/srand().
//
//Every draw NAMES what it is drawn for -- customRand("heightmap") -- and each
//name owns its OWN stream: a seed made of the world seed, the current salt and
//the name itself, plus a counter of how many times that name was already drawn.
//The value handed back is a hash of those, so it depends on nothing else -- not
//on how many numbers the rest of the run consumed.
//
//That is what makes a change reviewable: adding, removing or reordering a draw
//moves what is drawn for THAT name only, the rest of the world comes out byte
//for byte identical, so a diff shows the change and nothing else. With one
//global sequence, one extra draw shifted everything generated after it.
//
//THE REASON IS A STABLE IDENTITY, and it is what decides how far a change
//spreads. Normally a literal, one per call site: customRand("cave-item"). When
//the draw belongs to one OPTIONAL ASSET, the reason is that asset instead --
//customRand("template/sea/cargo-ship/how-use.ini") -- so that dropping another
//template folder in does not move what the ones already there roll. That is the
//whole point: a config or code change must move only what it is about.
//
//NEVER build the reason out of a value that changes from draw to draw (a
//coordinate, a loop index, a monster id). That is a stream per VALUE: each one
//starts at counter 0, so every call gives back the same number and the map fills
//with the same choice everywhere.
//
//It is DETERMINISTIC ACROSS PLATFORMS: nothing is hashed through its memory
//layout, so a big endian host generates the very same world as a little endian
//one, and customRandMax does not move with the C library the way RAND_MAX does.

//Highest value customRand() returns: the RAND_MAX of this generator, for the
//call sites that turn a draw into a fraction (customRand("x")/(double)customRandMax).
static const int customRandMax=0x7FFFFFFF;

//The world seed, read from the config file. Drops every stream.
void customRandSeed(const uint64_t &seed);

//Extra seed material mixed into every stream, and a reset of every counter:
//what srand() was called for, giving a CHUNK its own numbers so that what one
//chunk consumes cannot shift the content of the next one. Setting the same salt
//twice replays the same numbers.
void customRandSalt(const uint64_t &salt);

//One draw of the `reason` stream, in 0..customRandMax.
int customRand(const char * const reason);

#endif // CUSTOMRAND_H
