#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <functional>
#include <iostream>
#include <set>
#include <string>
#include <vector>

using namespace std::string_literals;

enum class Platform {
	Windows,
	Linux
};

constexpr bool debug = 
#ifdef _DEBUG
true
#else
false
#endif
;

constexpr Platform platform =
#ifdef _WIN32
Platform::Windows
#else
Platform::Linux
#endif
;

constexpr auto lf  = '\n';
constexpr auto tab = '\t';

template<typename Number>
constexpr bool nthBitHi(Number self, size_t n) {
	return self & (1 << n);
}

// Throws a runtime exception if Expr evaluates to true.
// The exception states the current source location,
// as well as the textual expression that was checked.
#define crashIf(Expr) { \
	if(Expr) { \
		std::stringstream message; \
		message << __FILE__ << ":" << __LINE__ << " '" << #Expr << '\''; \
		if constexpr(platform == Platform::Windows) { \
			std::cerr << message.str() << lf; \
		} \
		throw std::runtime_error(message.str()); \
	} \
}

// Maps an input range to a vector by applying the given mapping
// to each element inside it.
template<typename InputContainer, typename OutElem>
std::vector<OutElem> mapToVector(
	InputContainer const& elems, 
	std::function<OutElem(typename InputContainer::value_type const&)>&& mapping
) {
	std::vector<OutElem> result;
	result.reserve(std::size(elems));
	std::transform(std::begin(elems), std::end(elems), std::back_inserter(result), mapping);
	return std::move(result);
}

// Returns whether the given predicate is true for any item inside
// an input container. Optionally stores the found index, as well.
template<typename Container>
bool contains(
	Container const& container, 
	std::function<bool(typename Container::value_type const&)>&& pred,
	size_t* const outIndex = nullptr
) {
	auto position = std::find_if(std::begin(container), std::end(container), pred);
	if(position == std::end(container)) {
		return false;
	} else {
		if(outIndex) {
			*outIndex = std::distance(std::begin(container), position);
		}
		return true;
	}
}

// Constructs a vector containing the values [0,...,count).
template<typename Elem=size_t>
std::vector<Elem> range(size_t count) {
	std::vector<Elem> r;
	r.resize(count);
	for(size_t i=0; i<count; ++i) {
		r[i] = static_cast<Elem>(i);
	}
	return std::move(r);
}

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

	inline constexpr Value const* items() { return m_pItems; }
	inline constexpr size_t count() { return m_itemCount; }
	inline constexpr size_t bytes() { return m_totalBytes; }
};

template<typename Container>
constexpr View<typename Container::value_type> makeContainerView(Container const& container) {
	return View(std::data(container), std::size(container));
}