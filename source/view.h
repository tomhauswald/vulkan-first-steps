#pragma once

template<typename Value>
class View {
private:
	Value const* m_pItems;
	size_t m_itemCount;
	size_t m_totalBytes;

public:
	constexpr View(Value const* items, size_t count)
		: m_pItems(items),
		  m_itemCount(count),
		  m_totalBytes(count * sizeof(Value)) {
	}

	inline constexpr Value const* items() const { return m_pItems; }
	inline constexpr Value const& at(size_t i) const { return m_pItems[i]; }
	inline constexpr size_t count() const { return m_itemCount; }
	inline constexpr size_t bytes() const { return m_totalBytes; }
};

template<typename Container>
constexpr View<typename Container::value_type> makeContainerView(Container const& container) {
	return View(std::data(container), std::size(container));
}

template<typename Value>
constexpr View<Value> makeInlineView(std::initializer_list<Value> const& list) {
	return View(std::data(list), std::size(list));
}