/**
 * Hydrogen Operating System
 * Copyright (C) 2011 Lukas Heidemann
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// Some definitions...
void kmain(void);
void kmain_ap(void);

void screen_print(const char *str, unsigned int line);
void screen_clear(void);

unsigned char *videomem = (unsigned char *) 0xb8000;

/**
 * The main method, gets executed on the BSP.
 */
void kmain(void) {
    // Put code to run on BSP here
    screen_clear();
    screen_print("Hello World from Hydrogen C Kernel Example!", 2);
}

/**
 * The ap main method, gets executed on all APs.
 */
void kmain_ap(void) {
    // Put code to run on APs here
    while (1)
	    screen_print("AP!", 1);
}

/**
 * Clears the screen.
 */
void screen_clear(void) {
    unsigned int index;

    for (index = 0; index < 80 * 60; ++index) {
        videomem[index * 2    ] = ' ';
        videomem[index * 2 + 1] = 0xF;
    }
}

/**
 * Prints a string to a given line on the screen.
 *
 * @param str The string to print.
 * @param line The number of the line to print the string to.
 */
void screen_print(const char *str, unsigned int line) {
    unsigned int index = line * 80 * 2;

    while (0 != *str) {
        videomem[index    ] = *str;
        videomem[index + 1] = 0xF;

        str += 1;
        index += 2;
    }
}
