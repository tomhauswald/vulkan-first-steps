#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

constexpr bool debug = 
#ifdef _DEBUG
true
#else
false
#endif
;

constexpr auto lf  = '\n';
constexpr auto tab = '\t';

#define crash_if(Expr) { \
	if(Expr) { \
		throw std::runtime_error(std::string{__FILE__ ":"} + std::to_string(__LINE__) + std::string{" '" #Expr "'"}); \
	} \
}

template<typename InElem, typename OutElem>
std::vector<OutElem> vecToVec(std::vector<InElem> const& elems, std::function<OutElem(InElem const&)>&& mapping) {
	std::vector<OutElem> result;
	result.reserve(elems.size());
	std::transform(elems.begin(), elems.end(), std::back_inserter(result), mapping);
	return std::move(result);
}

