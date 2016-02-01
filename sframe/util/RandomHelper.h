#pragma once

#ifndef SFRAME_RANDOM_HELPER_H
#define SFRAME_RANDOM_HELPER_H

#include <random>

namespace sframe {

// [min_num, max_num)
void Rand(int min_num, int max_num, int count, std::vector<int> & random_numerbs);

// [min_num, max_num)
int Rand(int min_num, int max_num);

}

#endif