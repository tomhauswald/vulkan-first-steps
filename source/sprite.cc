#include "sprite.h"
#include "renderer.h"

void Sprite::draw(Renderer& r) {
    r.renderSprite(*this);
}