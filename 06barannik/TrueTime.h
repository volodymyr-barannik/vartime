#pragma once
#include <chrono>
#include "Algorithm.h"

using durations = std::tuple
<
	std::chrono::nanoseconds,
	std::chrono::microseconds,
	std::chrono::milliseconds,
	std::chrono::seconds,
	std::chrono::minutes,
	std::chrono::hours
>;

template <typename T>
struct duration_name;

template <> struct duration_name<std::chrono::nanoseconds>	{ static constexpr auto name = "nanoseconds"; };
template <> struct duration_name<std::chrono::microseconds> { static constexpr auto name = "microseconds"; };
template <> struct duration_name<std::chrono::milliseconds> { static constexpr auto name = "milliseconds"; };
template <> struct duration_name<std::chrono::seconds>		{ static constexpr auto name = "seconds"; };
template <> struct duration_name<std::chrono::minutes>		{ static constexpr auto name = "minutes"; };
template <> struct duration_name<std::chrono::hours>		{ static constexpr auto name = "hours"; };

template <typename T>
constexpr auto duration_name_v = duration_name<std::remove_const_t<std::remove_reference_t<T>>>::name;

// Represents a time in range of standard units [LowDurationType, HighDurationTime]
// e.g., Time<seconds, days> stores and expose seconds, minutes and days
template <class LowDurationType, class HighDurationType>
class Time
{
private:

	static_assert(std::ratio_less_equal_v<typename LowDurationType::period, typename HighDurationType::period>);

	template <typename L, typename H>
	friend class Time;

	using low_t		= LowDurationType;
	using high_t	= HighDurationType;

	template <typename Duration>
	struct is_within_current_range
	{
		static constexpr bool result =
			std::ratio_greater_equal_v
			<
			typename Duration::period,
			typename low_t::period
			>
			&& std::ratio_less_equal_v
			<
			typename Duration::period,
			typename high_t::period
			>;
	};
	
	using durations_t = filtered_tuple_t<is_within_current_range, durations>;
	durations_t _fields; // in order for copy-construction to work
						 // (it first initializes _fields with default values and then does its job)

	template <typename L1, typename L2>
	struct low_broader
	{
		using value = std::conditional_t
			<
			std::ratio_less_equal_v<typename L1::period, typename L2::period>,
			L1, L2
			>;
	};

	template <typename H1, typename H2>
	struct high_broader
	{
		using value = std::conditional_t
			<
			std::ratio_less_equal_v<typename H1::period, typename H2::period>,
			H2, H1
			>;
	};

	template <typename L1, typename L2>
	using low_broader_t = typename low_broader<L1, L2>::value;

	template <typename H1, typename H2>
	using high_broader_t = typename high_broader<H1, H2>::value;

	template <typename L, typename H, typename Callable>
	static consteval void apply_and_normalize(Time<L, H>& subject, Callable&& f)
	{
		for_each_idx(subject._fields, [&](auto&& current_dur_v, auto idx)
			{
				// 1. APPLY
				std::invoke(f, current_dur_v, idx);

				// 2. NORMALIZE
				// if next duration exists (it also will be "bigger" than current one)
				if constexpr (idx + 1 < std::tuple_size_v<durations_t>)
				{
					// then get its type
					using next_dur_t = typename std::remove_reference_t<decltype(std::get<idx + 1>(subject._fields))>;

					// and give it the surplus of current value (if exists)
					std::get<idx + 1>(subject._fields) += next_dur_t
					(
						// e.g. if we have curr_t = seconds, then next_t = minutes
						// and if we have, let's say, curr_v = 73, next_v = 4
						// then in this formula we'd get 73 / 60 = 1.
						// Thus, we have a surplus of value 1
						current_dur_v / next_dur_t(1)
					);
					// apply_and_normalize current value (take away the surplus)
					current_dur_v %= next_dur_t(1);
				}
			});
	}
	static consteval void normalize(Time&& subject)
	{
		apply_and_normalize(subject, [](auto&& current_dur_v, auto idx) {});
	}

public:

	consteval Time() = default;
	template <typename OtherL, typename OtherH>
	consteval Time(const Time<OtherL, OtherH>& other)
	{
		// Check whether construction is not a narrowing one
		static_assert(std::ratio_less_equal_v	<typename low_t::period,  typename OtherL::period> && "Narrowing construction is forbidden.");
		static_assert(std::ratio_greater_equal_v<typename high_t::period, typename OtherH::period> && "Narrowing construction is forbidden.");

		// copy type-wise
		copy_tuple_by_types<decltype(other._fields), decltype(_fields)>(other._fields, _fields);
	}

	// works nice with uniform initialization
	template <typename ...Args>
	inline consteval Time(Args... args) : _fields{ args... } {}
	inline consteval Time(durations_t fields) noexcept : _fields(fields) {}
	inline consteval Time(const LowDurationType& duration) { set<LowDurationType>(duration); }

	consteval Time& operator=(const Time& other) = default;
	constexpr ~Time() = default;

	inline constexpr const durations_t& getDurations() const noexcept { return _fields; }

	template <typename Duration>
	inline constexpr const Duration& get() const { return std::get<Duration>(_fields); }

	template <typename Duration>
	inline consteval void set(const Duration& value) const
	{	
		std::get<Duration>(_fields) = value;
		normalize(*this);
	}

	/*consteval Time operator+ (const Time& other) const
	{
		Time result = *this;
		apply_and_normalize(result, [&other](auto&& current_dur_v, auto idx)
			{
				current_dur_v += std::get<idx>(other._fields);
			});
		return result;
	}*/

	consteval Time operator+ (const Time& other) const
	{
		Time result = Time(*this);
		for_each_idx(result._fields, [&](auto&& f, auto idx)
			{
				f += std::get<idx>(other._fields);

				if constexpr (idx + 1 < std::tuple_size_v<durations_t>)
				{
					using next_dur = std::remove_reference_t< std::tuple_element_t<idx + 1, durations_t> >;

					std::get<idx + 1>(result._fields) += next_dur(f / next_dur(1));
					f %= next_dur(1);
				}
			});
		return result;
	}

	template <typename L, typename H>
	consteval auto operator+(const Time<L, H>& other) const
	{
		using BroadenedTime = Time<low_broader_t<low_t, L>, high_broader_t<high_t, H>>;
		return BroadenedTime(*this) + BroadenedTime(other);
	}
};


template <typename DurationLow, typename DurationHigh>
std::ostream& operator<<(std::ostream& os, const Time<DurationLow, DurationHigh>& time)
{
	os << "[ ";
	for_each(time.getDurations(), [&os](auto&& f) { os << f.count() << ' ' << duration_name_v<decltype(f)> << "; "; });
	os << ']';
	return os;
}
