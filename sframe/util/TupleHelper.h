
#ifndef SFRAME_TUPLE_HELPER_H
#define SFRAME_TUPLE_HELPER_H

#include <tuple>

namespace sframe {

// std::tuple Õ¹¿ª°ïÖúÀà
template<typename... Args>
class TupleUnfold
{
public:
	template<typename Receiver>
	static void Unfold(Receiver & recver, tuple<Args...> & t)
	{
		UnfoldHelper<tuple_size<tuple<Args...>>::value>::Unfold(recver, t);
	}

private:

	template<std::size_t N>
	struct UnfoldHelper
	{
		template<typename Receiver, typename... Tail>
		static void Unfold(Receiver & recver, tuple<Args...> & t, Tail&&... tail)
		{
			UnfoldHelper<N - 1>::Unfold(recver, t, get<N - 1>(t), tail...);
		}
	};


	template<>
	struct UnfoldHelper<0>
	{
		template<typename Receiver, typename... Tail>
		static void Unfold(Receiver & recver, tuple<Args...> & t, Tail&&... tail)
		{
			recver.Receive(std::forward<Tail>(tail)...);
		}
	};
};

}

#endif