#ifndef FILTER_H
#define FILTER_H

#include "TiledLayer.h"

class Filter
{
public:
    virtual ~Filter() = default;

    // Pure virtual function that child classes MUST implement
    virtual void apply(TiledLayer *layer) = 0;
};

#endif // FILTER_H