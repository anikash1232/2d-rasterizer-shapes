/*
 *  Copyright 2015 Mike Reed
 */

#ifndef GCanvas_DEFINED
#define GCanvas_DEFINED

#include "GPoint.h"
#include <string>

class GBitmap;
class GColor;
class GRect;

class GCanvas {
public:
    virtual ~GCanvas() {}

    /**
     *  Fill the entire canvas with the specified color, using SRC porter-duff mode.
     */
    virtual void clear(const GColor&) = 0;

    /**
     *  Draw a hairline from one point to the other, using the algorithm discussed
     *  in class. Be sure to "clip" the line to the bounds of the canvas.
     */
    virtual void hairLine(GPoint, GPoint, const GColor&) = 0;

    /**
     *  Fill the rectangle with the color, using SRC_OVER porter-duff mode.
     *
     *  The affected pixels are those whose centers are "contained" inside the rectangle:
     *      e.g. contained == center > min_edge && center <= max_edge
     *
     *  Any area in the rectangle that is outside of the bounds of the canvas is ignored.
     */
    virtual void fillRect(const GRect&, const GColor&) = 0;
};

/**
 *  Implemnt this, returning an instance of your subclass of GCanvas.
 */
std::unique_ptr<GCanvas> GCreateCanvas(const GBitmap&);

/**
 *  Implement this, drawing into the provided canvas, and returning the title of your artwork.
 */
std::string GDrawSomething(GCanvas* canvas, GISize dim);

#endif
