#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <functional>
#include <iostream>
#include <memory>
#include <set>
#include <string>
#include <vector>
#include <sstream>
#include <string_view>

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

template<typename Number>
constexpr bool nthBitHi(Number num, size_t n) {
	return num & (1 << n);
}

template<typename Number>
bool satisfiesBitMask(Number num, size_t mask) {
	return (num & mask) == mask;
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
	auto vec = std::vector<Elem>(count);
	for (size_t i = 0; i < count; ++i) {
		vec[i] = static_cast<Elem>(i);
	}
	return vec;
}

// Constructs a vector containing the same value repeatedly.
template<typename Elem>
std::vector<Elem> repeat(Elem const& value, size_t times) {
	auto vec = std::vector<Elem>(times);
	for (size_t i = 0; i < times; ++i) {
		vec[i] = value;
	}
	return vec;
}

template<typename Container>
size_t uniqueElemCount(Container const& container) {
	auto unique = std::set<typename Container::value_type>(
		std::begin(container), 
		std::end(container)
	);
	return unique.size();
}

inline float frand(float min, float max) {
	return min + (max - min) * rand() / (float)RAND_MAX;
};

#define GETTER(name, member) \
	inline decltype(member) const& name() const noexcept { \
		return member; \
	}

#define SETTER(name, member) \
	inline void name(decltype(member) value) noexcept { \
		member = std::move(value); \
	}