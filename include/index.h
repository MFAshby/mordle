#pragma once
#include "storage.h"
/**
 * Routines for rendering index page.
 */

/**
 * /game_state
 *   game state for the player we're rendering a page for.
 * /return  
 *   the rendered page.
 */ 
char* render_index(struct game_state game_state);