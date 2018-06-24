/** \file
A program to show Conway's game of life using the libtcod library and C
*/

/** \mainpage
Rules:
    * if live neighbours less than 2, cell dies
    * if live 2-3 live neighbours, cell lives
    * if live neighbours > 3, cell dies
    * if dead cell has exactly 3 live neighbours, lives

Jan Van Uytven (ysgard@gmail.com )
*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include "libtcod/libtcod.h"

#define MAP_WIDTH       300
/**< The width of the map in cells */
#define MAP_HEIGHT      80
/**< The height of the map in cells */
#define SCREEN_WIDTH    80
/**< The width of the viewport onto the map, i.e. the displayed window, in
 * cells */
#define SCREEN_HEIGHT   40
/**< The height of the viewport onto the map, i.e. the displayed window, in
 * cells */

#define FPS             30
/**< Clamp the FPS to the given value.*/

float noise_vert = 12.0;
float noise_hori = 40.0;
/**< The parameters for the Perlin noise used to produce the initial noise
 * map */

struct cell {
    bool alive;  /* 0 = dead, 1 = alive */
    int linger; /* A value between 0 and 9 */
    bool flip;   /* 0 = do not flip, 1 = flip */
} map[MAP_WIDTH][MAP_HEIGHT];
/**< The struct of a cell of the map.
 * alive - 0 is dead, 1 is alive
 * linger - used for cell coloring only, represents how long this cell has been
 * dead.
 * flip - Whether or not the cell should be flipped in the next tick.
 */

static int o_x = (MAP_WIDTH - SCREEN_WIDTH) / 2;
static int o_y = (MAP_HEIGHT - SCREEN_HEIGHT) / 2;

int map_x(int);
int map_y(int);
void init_game_map();
void display_map();
void tick();
void flip();
int live_die(int, int);
int live_neighbours(int, int);
void display_map();
int live_cells();
void init_noise();



int main(void) {

    int end_game = false;
    int fcount = 0;
    TCOD_key_t key;
    char status[SCREEN_WIDTH];

    /* Init game window */
    TCOD_console_set_custom_font("BrogueFont3.png", TCOD_FONT_TYPE_GREYSCALE|TCOD_FONT_LAYOUT_ASCII_INROW, 0, 0);
    TCOD_console_init_root(SCREEN_WIDTH, SCREEN_HEIGHT, "Conway's Game of Life", false, TCOD_RENDERER_SDL);
    TCOD_sys_set_fps(FPS);

    /* Initialize map */
    init_game_map();

    while(!end_game && !TCOD_console_is_window_closed()) {
        TCOD_console_flush();
        key = TCOD_console_check_for_keypress(TCOD_KEY_PRESSED);
        if (key.vk == TCODK_ESCAPE)
            end_game = true;
        tick();
        display_map();
        TCOD_console_print(NULL, 0, SCREEN_HEIGHT-1, "Frame: %d Cells: %d FPS: %d", ++fcount, live_cells(), TCOD_sys_get_fps());
    }

    return 0;
}

/** Create a perlin map and use it to determine a random noise distribution.
 *
 * This is a utility function called during map generation. */
void init_noise() {
    TCOD_noise_t noise2d = TCOD_noise_new(2, TCOD_NOISE_DEFAULT_HURST, TCOD_NOISE_DEFAULT_LACUNARITY, NULL);
    float p[2];
    int i, j;
    float noise;
    for (i = 0; i < MAP_WIDTH; ++i) {
        for(j = 0; j < MAP_HEIGHT; ++j) {
            p[0] = (i * noise_hori)/MAP_WIDTH;
            p[1] = (j * noise_vert)/MAP_HEIGHT;
            noise = TCOD_noise_get_ex(noise2d, p, TCOD_NOISE_PERLIN);
            if (noise >= 0) map[i][j].alive = true;
        }
    }
    TCOD_noise_delete(noise2d);
}

/** Count # of live cells on a map.  
 *
 * This is a debug function. */
int live_cells() {
    int i, j;
    int count = 0;
    for (i = 0; i < MAP_WIDTH; ++i) {
        for (j = 0; j < MAP_HEIGHT; ++j) {
            if (map[i][j].alive) count++;
        }
    }
    return count;
}

/** Update the map by flipping cells that have been toggled by live_die. */
void flip() {
    int i, j;
    for (i = 0; i < MAP_WIDTH; ++i) {
        for (j = 0; j < MAP_HEIGHT; ++j) {
            if (map[i][j].flip) {
                map[i][j].alive = map[i][j].alive ? false : true;
                map[i][j].flip = false;
            }
        }
    }
}

/** Return the amount of live neighbours
 *
 * \param x x-coordinate of the cell
 * \param y y-coordinate of the cell
 * \return The number of 'live' cells neighbouring this one. 
 */
int live_neighbours(int x, int y) {
    /* Return the amount of live neighbours */
    int li, lj, hi, hj, i, j;
    int count = 0;
    li = (x == 0) ? 0 : x - 1;
    lj = (y == 0) ? 0 : y - 1;
    hi = (x == MAP_WIDTH - 1) ? MAP_WIDTH - 1 : x + 1;
    hj = (y == MAP_HEIGHT - 1) ? MAP_HEIGHT - 1 : y + 1;
    for (i = li; i <= hi; i++) {
        for (j = lj; j <= hj; j++) {
            if (i == x && j == y) {
                continue;
            } else {
                if (map[i][j].alive) count++;
            }
        }
    }
    return count;
}

/** Given a cell at x, y; check its status and flip it depending 
 * on whether it lives or dies.
 *
 * \param x, y cell location
 * \return returns 1 if the cell got flipped, 0 otherwise
 */
int live_die(int x, int y) {
    /* Is it alive? */
    int n = live_neighbours(x, y);
    if (map[x][y].alive) {
        /* Check to see if it dies */
        if (n > 3 || n < 2) {
            /* printf("alive->dead, n is %d for %d, %d\n", n, x, y); */
            map[x][y].flip = true;
            return true;
        } else return false;
    } else {
        /* It's dead.  Does it live? */
        if (n == 3) {
            /* printf("dead->alive, n is %d for %d, %d\n", n, x, y); */
            map[x][y].flip = true;
            return true;
        } else {
            return false;
        }
    }
}

/** Update the map - first we flip cells depending on the rules (via live_die),
 * then we alter the cell linger value */
void tick() {
    int i, j;
    for (i = 0; i < MAP_WIDTH; ++i) {
        for (j = 0; j < MAP_HEIGHT; ++j) {
            live_die(i, j);
        }
    }
    /* flip the cells */
    flip();
    /* Update their linger values - live cells grow, dead cells shrink */
    for (i = 0; i < MAP_WIDTH; ++i) {
        for (j = 0; j < MAP_HEIGHT; ++j) {
            if (map[i][j].alive) {
                /* live cells grow to a maximum of 9 */
                    map[i][j].linger += map[i][j].linger == 9 ? 0 : 1;
            } else {
                /* dead cells shrink to a minimum of 0 */
                    map[i][j].linger -= map[i][j].linger == 0 ? 0 : 1;
            }
        }
    }
}

/** Display the portion of the map encompassed by the screen */
void display_map() {
    char c;
    int i, j;
    TCOD_color_t color_scale[] = {
        TCOD_black,
        TCOD_darkest_red,
        TCOD_darker_red,
        TCOD_dark_red,
        TCOD_red,
        TCOD_flame,
        TCOD_orange,
        TCOD_amber,
        TCOD_yellow,
        TCOD_light_yellow
    };
    for (i = 0; i < SCREEN_WIDTH; ++i) {
        for (j = 0; j < SCREEN_HEIGHT; ++j) {
            c = map[i+o_x][j+o_y].alive == true ? '*' : ' ';
            TCOD_console_put_char_ex(NULL, i, j, c, TCOD_white, color_scale[map[i+o_x][j+o_y].linger]);
        }
    }

    return;
}



/** Initialize the game map by populating it with a perlin noise map. */
void init_game_map() {

    int i, j, margin_w, margin_h, done;
    TCOD_mouse_t mouse;
    TCOD_key_t key;
    TCOD_event_t evt;

    /* Set all cells to nothing */
    for (i = 0; i < MAP_WIDTH; ++i) {
        for (j = 0; j < MAP_HEIGHT; ++j) {
            map[i][j] = (struct cell){.alive = false, .linger = 0, .flip = false};
        }
    }

    /* Create initial landscape */
    init_noise();
    display_map();
    TCOD_console_flush();


    done = false;
    while (!done) {
        /* Read mouse strokes and populate the map */
        evt = TCOD_sys_check_for_event(TCOD_EVENT_ANY, &key, &mouse);
        if ( evt == TCOD_EVENT_KEY_PRESS && key.vk == TCODK_SPACE ) {
            done = true;
        } else if ( evt == TCOD_EVENT_MOUSE_PRESS ) {
            map[mouse.cx+o_x][mouse.cy+o_y].alive = true;
        } else if ( evt == TCOD_EVENT_KEY_PRESS && key.vk == TCODK_ESCAPE ) {
            /* Quit program */
            exit(0);
        }
        display_map();
        TCOD_console_flush();
    }
    return;

}

