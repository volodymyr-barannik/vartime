#pragma once
#include <algorithm>
#include <tuple>
#include <functional>

// TUPLE

template<size_t i, class...Args>
using ith = std::tuple_element_t<i, std::tuple<Args...>>;

template<size_t i, typename... Args>
struct ith_type
{
	using type = ith<i, Args...>;
};

template<size_t i, typename... Args>
using ith_type_t = ith_type<i, Args...>::type;

// prepend element to tuple
template <typename Arg, class Tuple>
struct tuple_prepend {};

template <typename Arg, typename ...Args>
struct tuple_prepend<Arg, std::tuple<Args...>>
{
	using type = std::tuple<Arg, Args...>;
};

template <typename Arg, class Tuple>
using tuple_prepend_t = typename tuple_prepend<Arg, Tuple>::type;


// filter args
template <template <typename> class UnaryPredicate, typename ...Args>
struct filter_args;

template <template <typename> class UnaryPredicate, typename Arg, typename ...Args>
struct filter_args<UnaryPredicate, Arg, Args...>
{
	using type = std::conditional_t
	<
		UnaryPredicate<Arg>::value,													// if:   predicate holds true
		tuple_prepend_t<Arg, typename filter_args<UnaryPredicate, Args...>::type>,	// then: prepend current arg and proceed to next arg
		typename filter_args<UnaryPredicate, Args...>::type						// else: skip current arg and proceed to next arg
	>;
};

template <template <typename> class UnaryPredicate>
struct filter_args<UnaryPredicate>
{
	using type = std::tuple<>;
};

// filtered tuple
template <template <typename> class UnaryPredicate, class Tuple>
struct filtered_tuple {};

template <template <typename> class UnaryPredicate, typename ...Args>
struct filtered_tuple<UnaryPredicate, std::tuple<Args...>>
{
	using type = typename filter_args<UnaryPredicate, Args...>::type;
};

template <template <typename> class UnaryPredicate, class Tuple>
using filtered_tuple_t = typename filtered_tuple<UnaryPredicate, Tuple>::type;

// for each

// calls given Callable f for each of given args (preserving order of args)
template <class Callable, typename... Args>
constexpr Callable for_each_arg(Callable&& f, Args&& ...args)
{
	// cast to void in order to suppress warnings
	// this whole construction is needed for calling f with args in proper order -- and nothing more
	(void)std::initializer_list<int>
	{
		(
			(void)std::invoke(std::forward<Callable>(f), std::forward<Args>(args)), // call
			0 // just a lonely placeholder in order for this thing to work
			  // (initializer lists has to be constructed out of something,
			  //  so it shall be constructed out of zeros)
		)...
	};
	return f;
}

// calls given Callable f for each element of tuple
template <class Tuple, class Callable>
constexpr Callable for_each(Tuple&& on, Callable&& f)
{
	return std::apply
	(
		[&](auto&&... elem)			// function to be applied
		{
			return for_each_arg(std::forward<Callable>(f), std::forward<decltype(elem)>(elem)...);
		},
		std::forward<Tuple>(on)	// to what it shall be applied
	);
}

// calls given Callable f with each of given args (preserving order of args)
template <class Tuple, class Callable, std::size_t... idx>
constexpr Callable for_each_arg_idx(Tuple&& t, Callable&& f, std::index_sequence<idx...>)
{
	// for each given idx
	(void)std::initializer_list<int>
	{
		// call f for tuple's element with index idx
		((void)std::invoke
			(
				f,											// given function
				std::get<idx>(std::forward<Tuple>(t)),		// tuple element with index idx 
				std::integral_constant<std::size_t, idx>{}	// idx itself
			),
		0)...
	};
	return f;
}

// Consequently iterates over all elements of Tuple t, applying function f to each of them and
// "saving" index of that element in idx
// Callable should be of type [..](element, idx) -> void
// it's used like enumerate() in Python (kind of)
template <class Tuple, class Callable>
constexpr Callable for_each_idx(Tuple&& t, Callable&& f) {
	auto seq = std::make_index_sequence<
		std::tuple_size_v<std::remove_reference_t<Tuple>>>{};

	return for_each_arg_idx(std::forward<Tuple>(t),
		std::forward<Callable>(f),
		std::forward<decltype(seq)>(seq));
}

template <class Tuple, class Callable>
constexpr Callable for_each_idx(const Tuple&& t, Callable&& f) {
	auto seq = std::make_index_sequence<
		std::tuple_size_v<std::remove_reference_t<Tuple>>>{};

	return for_each_arg_idx(std::forward<Tuple>(t),
		std::forward<Callable>(f),
		std::forward<decltype(seq)>(seq));
}

// requires types of elements of both tuples to be unique
template <typename TupleFrom, typename TupleTo>
constexpr void copy_tuple_by_types(const TupleFrom& from, TupleTo& to)
{
	for_each(from, [&to](auto&& elem)
		{
			// Decay the type of elem. It's necessary as far as elem is const because it's contained in const std::tuple.
			// So in order to modify element of the same type contained in target tuple (which is not constant!)
			// we have to... decay it. Throw everything away.
			std::get<std::decay_t<decltype(elem)>>(to) = elem;
		});
}

// requires types of elements of both tuples to be unique
template <typename TupleFrom, typename TupleTo, typename TupleBy>
constexpr void copy_tuple_by_types(const TupleFrom& from, TupleTo& to, const TupleBy& by)
{
	for_each(by, [&from, &to](auto&& elem)
		{
			using decayed_elem = typename std::decay_t<decltype(elem)>;
			std::get<decayed_elem>(to) = std::get<decayed_elem>(from);
		});
}