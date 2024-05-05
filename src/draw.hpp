#ifndef _DRAW_H_
#define _DRAW_H_

typedef unsigned int uint;

/**
 * TODO: DOC
 * @param x
 * @param y
 * @param w
 * @param h
 * @param r0
 * @param color
 */
void draw_rounded_rectangle(uint x, uint y, uint w, uint h, uint r0,
                            uint color);

/**
 * TODO: DOC
 * @param x
 * @param y
 * @param w
 * @param h
 * @param r0
 * @param r1
 * @param color
 */
void draw_rounded_rectangle(uint x, uint y, uint w, uint h, uint r0, uint r1,
                            uint color);

/**
 * TODO: DOC
 * @param x
 * @param y
 * @param w
 * @param h
 * @param r0
 * @param r1
 * @param r2
 * @param color
 */
void draw_rounded_rectangle(uint x, uint y, uint w, uint h, uint r0, uint r1,
                            uint r2, uint color);

/**
 * TODO: DOC
 * @param x
 * @param y
 * @param w
 * @param h
 * @param r0
 * @param r1
 * @param r2
 * @param r3
 * @param color
 */
void draw_rounded_rectangle(uint x, uint y, uint w, uint h, uint r0, uint r1,
                            uint r2, uint r3, uint color);

#endif
