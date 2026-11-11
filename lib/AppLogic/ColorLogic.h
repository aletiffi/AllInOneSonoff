#ifndef ColorLogic_h
#define ColorLogic_h

// Ledstrip color and effects logic, independent from Arduino (unit tested on PC)

#include <stdint.h>

#define EFFECT_SOLID              0
#define EFFECT_RANDOM             1
#define EFFECTS_NUM               2
#define RANDOM_EFFECT_STEPS       9

struct Rgb {
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

// "#rrggbb" (upper or lower case) -> rgb. Returns false (and leaves rgb unchanged) if not valid
bool color_from_hex(const char* hex, Rgb* rgb);
// rgb -> "#rrggbb", out must be at least 8 chars
void color_to_hex(Rgb rgb, char* out);

// Effects by name: "Solid", "Random". Returns -1 if unknown
int effect_from_name(const char* name);
const char* effect_name(int effect);

// Moves value towards target by step; reached is true when value was already equal to target
uint8_t color_channel_step(uint8_t value, uint8_t target, uint8_t step, bool* reached);

// Random effect: each step moves one channel to max, min or half value.
// rnd(n) must return a number in [0, n)
typedef uint8_t (*RandomFn)(uint8_t n);

// Chooses the next step avoiding to switch off the strip (never two channels at 0 and the third going to 0)
uint8_t random_effect_next_step(Rgb color, RandomFn rnd);
// One tick of the effect: updates color and, when the target is reached, *step
void random_effect_tick(uint8_t* step, Rgb* color, uint8_t incStep, RandomFn rnd);

#endif
