
#include <time.h>
#include <assert.h>
#include "RandomHelper.h"

using namespace sframe;

inline std::default_random_engine& GetEngine()
{
    static std::default_random_engine eng(static_cast<int>(time(nullptr)));
    return eng;
}

void sframe::Rand(int min_num, int max_num, int count, std::vector<int> & random_numerbs)
{
    assert(max_num > min_num);
    auto& eng = GetEngine();
    std::uniform_int_distribution<int> u(min_num, max_num - 1);
    for (int i = 0; i < count; ++i)
    {
        random_numerbs.push_back(u(eng));
    }
}

int sframe::Rand(int min_num, int max_num)
{
    assert(max_num > min_num);
    auto& eng = GetEngine();
    return std::uniform_int_distribution<int>(min_num, max_num - 1)(eng);
}
