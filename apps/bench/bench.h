/**
 *  Copyright 2016 Mike Reed
 */

#ifndef _bench_h_DEFINED
#define _bench_h_DEFINED

#include "../auto_register.h"
#include "../../include/GCanvas.h"
#include "../../include/GColor.h"
#include "../../include/GPoint.h"
#include "../../include/GRandom.h"
#include "../../include/GRect.h"
#include <memory>

class GBenchmark {
public:
    virtual ~GBenchmark() {}

    virtual const char* name() const = 0;
    virtual GISize size() const = 0;
    virtual void draw(GCanvas*) = 0;
};

using GBenchFact = GBenchmark*();

struct GBenchRec {
    GBenchFact* fFactory;
    int         fPA;
};

using BenchRegistrant = GRegistrant<GBenchRec>;
#define REGISTER_GBENCH(pa, code) \
    static BenchRegistrant G_MACRO_UNIQUE_NAME(gbench_proc)({ []() -> GBenchmark* { code }, pa })

#endif
