#ifndef CG_H
#define CG_H

#include <Arduino.h>

// Initialize CG animation (reset phase and timing)
void cgInit();

// Update CG animation (non-blocking, returns true when animation finishes)
bool cgUpdate();

// Draw the current CG frame (full scene, should only be called when dirty)
void cgDraw();

// Returns true (once) when the phase just changed and a full redraw is needed
bool cgNeedsRedraw();

// Per-frame incremental draw: typewriter reveal + blood drip animation (phase 0 only)
void cgDrawUpdate();

#endif