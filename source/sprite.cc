#include "sprite.h"
#include "renderer.h"

void Sprite::draw(Renderer& r) const {
    r.renderSprite(*this);
}