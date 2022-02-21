#pragma once
#include "storage.h"
/**
 * Routines for rendering index page.
 */

#define max_flash 100
/**
 * /game_state
 *   game state for the player we're rendering a page for.
 * /game_user
 *   the player!
 * /wordle
 *   today's wordle
 * /flash
 *   a flash message to show the user, typically a one-time validation message like 'invalid word guessed!'
 * /return  
 *   the rendered page.
 */ 
char* render_index(struct game_state game_state, 
    struct game_user game_user, 
    struct wordle wordle,
    struct leaderboard_top10 leaderboard,
    char flash[max_flash]);