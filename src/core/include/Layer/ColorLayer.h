#ifndef COLORLAYER_H
#define COLORLAYER_H

#include "TiledLayer.h"

class ColorLayer : public TiledLayer
{
public:
    ColorLayer(int w, int h, const std::string &n);

    void setPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a) override;

    void getPixel(int x, int y, uint8_t &r, uint8_t &g, uint8_t &b, uint8_t &a) const override;
    void fill(uint8_t r, uint8_t g, uint8_t b, uint8_t a) override;

    virtual Layer::Type getType() const override { return Layer::Type::Color; }
};
#endif // COLORLAYER_H