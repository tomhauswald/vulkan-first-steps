#pragma once

#include "game_object.h"
#include "sprite.h"

template <typename TileType>
class Tilemap : public GameObject {
 private:
  glm::u64vec3 m_size;
  std::vector<TileType> m_tiles;

  Texture const& m_tileset;
  glm::size_t m_srcTilesPerRow;
  glm::size_t m_srcTilesPerCol;
  glm::vec2 m_dstTileSize;
  glm::vec2 m_uvIncrement;

  Sprite m_tileSprite;

  inline size_t tileIndex(glm::u64vec3 const& pos) const {
    crashIf(pos.x > m_size.x || pos.y > m_size.y || pos.z > m_size.z);
    return static_cast<size_t>(pos.x) + (pos.y + pos.z * m_size.y) * m_size.x;
  }

  inline void setTileAt(size_t index, TileType value) {
    crashIf(value > m_srcTilesPerCol * m_srcTilesPerRow);
    m_tiles[index] = std::move(value);
  }

 public:
  inline Tilemap(glm::u64vec3 size, Texture const& tileset,
                 glm::uvec2 const& srcTileSize, glm::vec2 dstTileSize)
      : m_size{std::move(size)},
        m_tiles(m_size.x * m_size.y * m_size.z),
        m_tileset{tileset},
        m_srcTilesPerRow{m_tileset.width() / srcTileSize.x},
        m_srcTilesPerCol{m_tileset.height() / srcTileSize.y},
        m_dstTileSize{std::move(dstTileSize)},
        m_uvIncrement{srcTileSize.x / static_cast<float>(m_tileset.width()),
                      srcTileSize.y / static_cast<float>(m_tileset.height())},
        m_tileSprite{tileset} {}

  inline virtual ~Tilemap() {}

  inline TileType const& tileAt(glm::u64vec3 const& pos) const noexcept {
    return m_tiles[tileIndex(pos)];
  }

  inline void setTileAt(glm::u64vec3 const& pos, TileType value) {
    setTileAt(tileIndex(pos), std::move(value));
  }

  inline void fill(size_t layer, TileType value) {
    crashIf(layer >= m_size.z);

    auto tilesPerLayer = m_size.x * m_size.y;
    auto layerStart = layer * tilesPerLayer;

    for (auto i : range(tilesPerLayer)) {
      setTileAt(layerStart + i, std::move(value));
    }
  }

  inline void clear(size_t layer) { fill(layer, 0); }

  inline void clear() {
    for (auto z : range(m_size.z)) {
      clear(z);
    }
  }

  inline virtual void update(float dt) override { GameObject::update(dt); }

  inline virtual void draw(Renderer& r) const override {
    auto minLoc = glm::u64vec3{r.camera2d().position() / m_dstTileSize, 0};
    auto visibleArea =
        glm::u64vec2{glm::ceil(r.camera2d().viewportSize() / m_dstTileSize)};
    auto maxLoc = glm::u64vec3{
        std::min(m_size.x - 1, minLoc.x + visibleArea.x - 1),
        std::min(m_size.y - 1, minLoc.y + visibleArea.y - 1), m_size.z - 1};

    auto sprite = m_tileSprite;
    for (size_t z = minLoc.z; z <= maxLoc.z; ++z) {
      for (size_t y = minLoc.y; y <= maxLoc.y; ++y) {
        for (size_t x = minLoc.x; x <= maxLoc.x; ++x) {
          auto value = tileAt({x, y, z});
          if (value) {
            sprite.setSize(m_dstTileSize);
            sprite.setPosition(m_dstTileSize * glm::vec2{x, y});
            sprite.setTextureArea(
                {m_uvIncrement.x * ((value - 1) % m_srcTilesPerRow),
                 m_uvIncrement.y * ((value - 1) / m_srcTilesPerRow),
                 m_uvIncrement});
            sprite.setLayer(z);
            r.renderSprite(sprite);
          }
        }
      }
    }
  }

  GETTER(size, m_size)
  GETTER(tileSize, m_dstTileSize)
};

using Tilemap8 = Tilemap<uint8_t>;
using Tilemap16 = Tilemap<uint16_t>;
using Tilemap32 = Tilemap<uint32_t>;
using Tilemap64 = Tilemap<uint64_t>;