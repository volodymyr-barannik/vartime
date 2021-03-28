#pragma once
#include <algorithm>
#include <tuple>
#include <functional>
#include <iostream>

// TUPLE

// has type
template <typename Type, typename Tuple>
struct has_type;

template <typename Type, typename... Types>
struct has_type<Type, std::tuple<Types...>> : std::disjunction<std::is_same<Type, Types>...> {};

template <typename Type, typename Tuple>
using has_type_t = typename has_type<Type, Tuple>::type;

template <typename Type, typename Tuple>
constexpr static bool has_type_v = has_type<Type, Tuple>::type::value;


// i-th type
template<size_t i, class...Args>
using ith = std::tuple_element_t<i, std::tuple<Args...>>;

template<size_t i, typename... Args>
struct ith_type
{
	using type = ith<i, Args...>;
};

template<size_t i, typename... Args>
using ith_type_t = typename ith_type<i, Args...>::type;


// prepend element to tuple
template <typename Arg, class Tuple>
struct tuple_prepend;

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
		typename filter_args<UnaryPredicate, Args...>::type							// else: skip current arg and proceed to next arg
	>;
};

template <template <typename> class UnaryPredicate>
struct filter_args<UnaryPredicate>
{
	using type = std::tuple<>;
};

// filtered tuple
template <template <typename> class UnaryPredicate, class Tuple>
struct filtered_tuple;

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


// requires Callable of type [](auto&& idx) {...};
template <class Callable, std::size_t... idx>
constexpr Callable for_each_arg_idx(Callable&& f, std::index_sequence<idx...>)
{
	// for each given idx
	(void)std::initializer_list<int>
	{
		// call f
		((void)std::invoke(f, std::integral_constant<std::size_t, idx>{}), 0)...
	};
	return f;
}

template <class Tuple, class Callable>
constexpr Callable for_each_type(Callable&& f) {
	auto seq = std::make_index_sequence<
		std::tuple_size_v<std::remove_reference_t<Tuple>>>{};

	return for_each_arg_idx(std::forward<Callable>(f),
							std::forward<decltype(seq)>(seq));
}




// requires types of elements of both tuples to be unique
template <class TupleBy, class TupleTo, class TupleFrom>
constexpr void copy_tuple_by_types(TupleTo& to, const TupleFrom& from, const TupleBy& by = TupleBy{})
{
	for_each(by, [&from, &to](auto&& elem)
		{
			using decayed_elem = typename std::decay_t<decltype(elem)>;
			std::get<decayed_elem>(to) = std::get<decayed_elem>(from);
		});
}

// Unfortunately it is impossible to make it work when evaluated at runtime.
// tuple_element_t can be evaluated only at compile-time.
// There's no runtime reflection in C++, so we can't acquire type of i'th element of tuple at runtime.
// Thus, we have to create a "proxy" tuple and then iterate over its elements.
// Please, please, please. Tell me I'm wrong. Tell me there's another way of doing it. Is there any such?
// 
//template <class TupleBy, class TupleTo, class TupleFrom>
//constexpr void copy_tuple_by_types(TupleTo& to, const TupleFrom& from)
//{
//	for_each_type<TupleBy>([&from, &to](auto&& idx)
//		{
//			using decayed_elem = typename std::decay_t<std::tuple_element_t<idx, TupleBy>>;
//			std::get<decayed_elem>(to) = std::get<decayed_elem>(from);
//		});
//}


template<class Tuple1, class Tuple2>
struct intersect_ordered_tuples
{
	template<typename Type>
	struct intersect_ordered_tuples_predicate
	{
		constexpr static bool value = has_type_v<std::decay_t<Type>, std::decay_t<Tuple2>>;
	};

	using type = filtered_tuple_t<intersect_ordered_tuples_predicate, Tuple1>;
};

template<typename Tuple1, typename Tuple2>
using intersect_ordered_tuples_t = typename intersect_ordered_tuples<Tuple1, Tuple2>::type;
