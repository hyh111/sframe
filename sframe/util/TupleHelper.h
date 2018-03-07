
#ifndef SFRAME_TUPLE_HELPER_H
#define SFRAME_TUPLE_HELPER_H

#include <tuple>

namespace sframe {


template<std::size_t N>
struct UnfoldHelper
{
	template<typename Obj, typename Tuple, typename... Tail>
	static auto Unfold(Obj * obj, Tuple & t, Tail&&... tail)
		-> decltype(UnfoldHelper<N - 1>::Unfold(obj, t, std::get<N - 1>(t), tail...))
	{
		return UnfoldHelper<N - 1>::Unfold(obj, t, std::get<N - 1>(t), tail...);
	}
};


template<>
struct UnfoldHelper<0>
{
	template<typename Obj, typename Tuple, typename... Tail>
	static auto Unfold(Obj * obj, Tuple & t, Tail&&... tail)
		-> decltype(obj->DoUnfoldTuple(std::forward<Tail>(tail)...))
	{
		return obj->DoUnfoldTuple(std::forward<Tail>(tail)...);
	}
};

// 展开std::tuple
template<typename Obj, typename Tuple>
inline auto UnfoldTuple(Obj * obj, Tuple & t)
	-> decltype(UnfoldHelper<std::tuple_size<Tuple>::value>::Unfold(obj, t))
{
	return UnfoldHelper<std::tuple_size<Tuple>::value>::Unfold(obj, t);
}

}

#endif