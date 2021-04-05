#pragma once
#include <algorithm>
#include <tuple>
#include <functional>

// TUPLE

// has type
template <typename Type, class Tuple>
struct has_type;

template <typename Type, typename... TupleTypes>
struct has_type<Type, std::tuple<TupleTypes...>> : std::disjunction<std::is_same<Type, TupleTypes>...> {};

template <typename Type, class Tuple>
using has_type_t = typename has_type<Type, Tuple>::type;

template <typename Type, class Tuple>
constexpr static bool has_type_v = has_type<Type, Tuple>::type::value;


// i-th type
template<size_t i, typename... Args>
struct ith_type
{
	using type = std::tuple_element_t<i, std::tuple<Args...>>;
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


// filter elements
template <template <typename> class UnaryPredicate, typename ...Elemenets>
struct filter_elements;

template <template <typename> class UnaryPredicate, typename CurrentElement, typename ...Elements>
struct filter_elements<UnaryPredicate, CurrentElement, Elements...>
{
	using type = std::conditional_t
	<
		UnaryPredicate<CurrentElement>::value,															// if:   predicate holds true
		tuple_prepend_t<CurrentElement, typename filter_elements<UnaryPredicate, Elements...>::type>,	// then: prepend current arg and proceed to next element
		typename filter_elements<UnaryPredicate, Elements...>::type										// else: skip current element and proceed to the next one
	>;
};

template <template <typename> class UnaryPredicate>
struct filter_elements<UnaryPredicate>
{
	using type = std::tuple<>;
};

// filtered tuple
template <template <typename> class UnaryPredicate, class Tuple>
struct filtered_tuple;

template <template <typename> class UnaryPredicate, typename ...Elements>
struct filtered_tuple<UnaryPredicate, std::tuple<Elements...>>
{
	using type = typename filter_elements<UnaryPredicate, Elements...>::type;
};

template <template <typename> class UnaryPredicate, class Tuple>
using filtered_tuple_t = typename filtered_tuple<UnaryPredicate, Tuple>::type;

// for each

// calls given Callable f for each of given args (preserving order of args)
template <class Callable, typename... Elements>
constexpr Callable for_each_arg(Callable&& f, Elements&& ...args)
{
	// cast to void in order to suppress warnings
	// this whole construction is needed for calling f with args in proper order -- nothing more than that
	(void)std::initializer_list<int>
	{
		(
			(void)std::invoke(std::forward<Callable>(f), std::forward<Elements>(args)), // call
			0 // just a lonely placeholder in order for this thing to work
			  // (initializer lists has to be constructed out of something,
			  //  so it shall be constructed out of zeros
			  //  because 0 is a result of expression of type (X, 0))
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

// Sequentially iterates over all elements of Tuple t, applying function f to each of them and
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

// For each element type in "by" copy element with that exact type from "from" to "to"
// NB! Requires types of elements of both tuples to be unique.
template <class TupleBy, class TupleTo, class TupleFrom>
constexpr void copy_tuple_by_types(TupleTo& to, const TupleFrom& from, const TupleBy& by = TupleBy{})
{
	for_each(by, [&from, &to](auto&& elem) {
			using decayed_elem = typename std::decay_t<decltype(elem)>;
			std::get<decayed_elem>(to) = std::get<decayed_elem>(from);
		});
}

// Unfortunately, as far as I know, it is [nearly] impossible to make following code work when evaluated at runtime.
// tuple_element_t can be evaluated at compile-time only.
// There's no runtime reflection in C++, so we can't acquire type of i'th element of tuple at runtime.

// Thus, we have to create a "proxy" tuple and then iterate over its elements (that's what we did in the function above)

// Another way would be to generate a lookup table for each tuple we create
// such that it maps each element index to its memory address.
// But that might complicate stuff way too much if we want to make things neat and efficient and
// not to bloat size of executables.


// requires Callable of form [....](auto&& idx) {....};
//template <class Callable, std::size_t... idx>
//constexpr Callable for_each_arg_idx(Callable&& f, std::index_sequence<idx...>)
//{
//	// for each given idx
//	(void)std::initializer_list<int>
//	{
//		// call f
//		((void)std::invoke(f, std::integral_constant<std::size_t, idx>{}), 0)...
//	};
//	return f;
//}
//
//template <class Tuple, class Callable>
//constexpr Callable for_each_type(Callable&& f)
//{
//	auto seq = std::make_index_sequence<
//		std::tuple_size_v<std::remove_reference_t<Tuple>>>{};
//
//	return for_each_arg_idx(std::forward<Callable>(f),
//		std::forward<decltype(seq)>(seq));
//}

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

template<class Tuple1, class Tuple2>
using intersect_ordered_tuples_t = typename intersect_ordered_tuples<Tuple1, Tuple2>::type;
