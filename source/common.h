#pragma once

#include <array>
#include <cmath>
#include <string>
#include <vector>
#include <iostream>

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