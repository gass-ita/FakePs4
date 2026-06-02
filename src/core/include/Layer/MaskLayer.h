#ifndef MASKLAYER_H
#define MASKLAYER_H

#include "TiledLayer.h"

class MaskLayer : public TiledLayer
{
public:
    MaskLayer(int w, int h, const std::string &n);

    // Mask only cares about Alpha (a)
    void setPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a) override;

    void getPixel(int x, int y, uint8_t & /*r*/, uint8_t & /*g*/, uint8_t & /*b*/, uint8_t & /*a*/) const override;
    void fill(uint8_t /*r*/, uint8_t /*g*/, uint8_t /*b*/, uint8_t a) override;
    virtual Layer::Type getType() const override { return Layer::Type::Mask; }
};

#endif // MASKLAYER_H