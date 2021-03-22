#include <iostream>
#include <chrono>
#include <tuple>

using durations = std::tuple<
	std::chrono::nanoseconds,
	std::chrono::microseconds,
	std::chrono::milliseconds,
	std::chrono::seconds,
	std::chrono::minutes,
	std::chrono::hours
>;

template <typename T>
struct duration_name;

template <> struct duration_name<std::chrono::nanoseconds> { static constexpr auto name = "nanoseconds"; };
template <> struct duration_name<std::chrono::microseconds> { static constexpr auto name = "microseconds"; };
template <> struct duration_name<std::chrono::milliseconds> { static constexpr auto name = "milliseconds"; };
template <> struct duration_name<std::chrono::seconds> { static constexpr auto name = "seconds"; };
template <> struct duration_name<std::chrono::minutes> { static constexpr auto name = "minutes"; };
template <> struct duration_name<std::chrono::hours> { static constexpr auto name = "hours"; };

template <typename T>
constexpr auto duration_name_v = duration_name<std::remove_reference_t<T>>::name;

template <typename Arg, class Tuple>
struct tuple_cons;

template <typename Arg, typename ...Args>
struct tuple_cons<Arg, std::tuple<Args...>> {
	using result = std::tuple<Arg, Args...>;
};

template <typename Arg, class Tuple>
using tuple_cons_t = typename tuple_cons<Arg, Tuple>::result;

template <template <typename> class Filter, typename ...Args>
struct filter_args;

template <template <typename> class Filter, typename Arg, typename ...Args>
struct filter_args<Filter, Arg, Args...> {
	using result = std::conditional_t<
		Filter<Arg>::result,
		tuple_cons_t<Arg, typename filter_args<Filter, Args...>::result>,
		typename filter_args<Filter, Args...>::result
	>;
};

template <template <typename> class Filter>
struct filter_args<Filter> { using result = std::tuple<>; };

template <template <typename> class Filter, class Tuple>
struct filter_tuple;

template <template <typename> class Filter, typename ...Args>
struct filter_tuple<Filter, std::tuple<Args...>> {
	using result = typename filter_args<Filter, Args...>::result;
};

template <template <typename> class Filter, class Tuple>
using filter_tuple_t = typename filter_tuple<Filter, Tuple>::result;

template <class T, class Tuple>
struct index_of;

template <class T, class... Types>
struct index_of<T, std::tuple<T, Types...>> {
	static constexpr std::size_t value = 0;
};

template <class T, class U, class... Types>
struct index_of<T, std::tuple<U, Types...>> {
	static constexpr std::size_t value = 1 + index_of<T, std::tuple<Types...>>::value;
};


template <class Low, class High>
struct Time {
	static_assert(std::ratio_less_v<typename Low::period, typename High::period>);

	template <typename Duration>
	struct is_within {
		static constexpr bool result = std::ratio_greater_equal_v<
			typename Duration::period,
			typename Low::period
		> && std::ratio_less_equal_v<
			typename Duration::period,
			typename High::period
		>;
	};

	using fields_type = filter_tuple_t<is_within, durations>;

	fields_type fields;

	template <typename ...Args>
	consteval Time(Args... args) : fields{ args... }
	{}

	template <typename L2, typename H2>
	consteval Time(const Time<L2, H2>& other)
	{
		static_assert(std::ratio_less_equal_v< typename Low::period, typename L2::period >);
		static_assert(std::ratio_greater_equal_v< typename High::period, typename H2::period >);

		constexpr int offset = index_of<L2, fields_type>::value;

		for_each_idx(other.fields, [&](auto&& f, auto idx)
			{
				std::get<idx + offset>(fields) = f;
			});
	}

	consteval Time operator+ (const Time& other) const
	{
		Time result = *this;
		for_each_idx(result.fields, [&](auto&& f, auto idx)
			{
				f += std::get<idx>(other.fields);

				if constexpr (idx + 1 < std::tuple_size_v<fields_type>)
				{
					using next_dur = std::remove_reference_t< std::tuple_element_t<idx + 1, fields_type> >;

					std::get<idx + 1>(result.fields) += next_dur(f / next_dur(1));
					f %= next_dur(1);
				}
			});
		return result;
	}

	template <typename L2, typename H2>
	consteval auto operator+ (const Time<L2, H2>& other) const
	{
		using ResL = std::conditional_t< std::ratio_less_equal_v< typename Low::period, typename L2::period >, Low, L2>;
		using ResH = std::conditional_t< std::ratio_greater_equal_v< typename High::period, typename H2::period >, High, H2>;
		using ResultType = Time<ResL, ResH>;
		return ResultType(*this) + ResultType(other);
	}
};

// for each

template <class F, typename ...Args>
consteval F for_each_arg(F&& f, Args&& ...args) {
	(void)std::initializer_list<int>{((void)f(std::forward<Args>(args)), 0)...};
	return f;
}

template <class Tuple, class F>
consteval F for_each(Tuple&& t, F&& f) {
	return std::apply(
		[&](auto&& ...as) {
			return for_each_arg(std::forward<F>(f), std::forward<decltype(as)>(as)...);
		},
		std::forward<Tuple>(t)
			);
}

template <class Tuple, class F, std::size_t ...Idx>
consteval F for_each_arg_idx(Tuple&& t, F&& f, std::index_sequence<Idx...>) {
	(void)std::initializer_list<int>{((void)f(
		std::get<Idx>(std::forward<Tuple>(t)),
		std::integral_constant<std::size_t, Idx>{}
	), 0)...};
	return f;
}

template <class Tuple, class F>
consteval F for_each_idx(Tuple&& t, F&& f) {
	auto seq = std::make_index_sequence<
		std::tuple_size_v<std::remove_reference_t<Tuple>>
	>{};
	return for_each_arg_idx(std::forward<Tuple>(t), std::forward<F>(f), seq);
}

int main() {
	using namespace std::literals::chrono_literals;

	using Tm1 = Time<std::chrono::microseconds, std::chrono::minutes>;
	using Tm2 = Time<std::chrono::milliseconds, std::chrono::seconds>;

	constexpr Tm1 t1{ 10us, 30ms, 0s, 5min };
	constexpr Tm2 t2{ 980ms, 5s };

	constexpr auto r = t1 + t2;

	//static_assert(std::get<std::chrono::seconds>(r.fields) == 6s);
}
