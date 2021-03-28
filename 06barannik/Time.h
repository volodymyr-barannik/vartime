#pragma once
#include <chrono>
#include <iostream>
#include <time.h>
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
constexpr auto duration_name_v = duration_name<std::decay_t<T>>::name;

// Represents a time in range of standard units [LowDurationType, HighDurationTime]
// e.g., Time<seconds, days> stores and exposes seconds, minutes and days
template <class LowDurationType, class HighDurationType = LowDurationType>
class Time
{
private:

	template <typename L, typename H>
	friend class Time;

	using low_t		= LowDurationType;
	using high_t	= HighDurationType;

	template <typename Duration>
	struct is_within_current_range
	{
		static constexpr bool value =
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
	durations_t _durations;

	template <typename L1, typename L2>
	struct low_broader
	{
		using value = std::conditional_t < std::ratio_less_equal_v<typename L1::period, typename L2::period>,
			L1, L2>;
	};

	template <typename H1, typename H2>
	struct high_broader
	{ 
		using value = std::conditional_t<std::ratio_less_equal_v<typename H1::period, typename H2::period>, 
			H2, H1>; 
	};

	template <typename L1, typename L2>
	using low_broader_t = typename low_broader<L1, L2>::value;

	template <typename H1, typename H2>
	using high_broader_t = typename high_broader<H1, H2>::value;

	template <typename L, typename H, typename Callable>
	static constexpr void apply_and_normalize(Time<L, H>& subject, Callable&& f)
	{
		for_each_idx(subject._durations, [&](auto&& current_dur_v, auto idx)
			{
				// 1. APPLY
				std::invoke(f, current_dur_v, idx);

				// 2. NORMALIZE
				// if next duration exists (it also will be "greater" than current one)
				if constexpr (idx + 1 < std::tuple_size_v<durations_t>)
				{
					// then get its type
					using next_dur_t = typename std::remove_reference_t<decltype(std::get<idx + 1>(subject._durations))>;

					// and give it the surplus of current value (if exists)
					std::get<idx + 1>(subject._durations) += next_dur_t
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

	template <typename L, typename H>
	static constexpr void normalize(Time<L, H>& subject)
	{
		apply_and_normalize(subject, [](auto&& current_dur_v, auto idx) {});
	}

	using DefaultTime = Time<std::chrono::seconds, std::chrono::hours>;

public:

	static_assert(std::ratio_less_equal_v<typename LowDurationType::period, typename HighDurationType::period> && "Low bound cannot exceed high bound");
	// TODO: add check that every next is greater than prev

	// Creates Time out of given durations
	template <typename... Durations>
	constexpr Time(const Durations&... durations) : _durations{ durations... } {}

	// TODO: Consider suprplus flattening ([5s, 80min] -> [5s, ???min, ???h]. I want it to be [5s, 20min, 1h]
	// Creates a copy of given Time object and truncates/converts it if needed
	template <typename L, typename H>
	constexpr Time(const Time<L, H>& other)
	{
		if constexpr (std::ratio_less_v		<typename high_t::period, typename H::period> ||
					  std::ratio_greater_v	<typename low_t::period,  typename L::period>)
		{
			// conversion that truncates smaller units
			// 
			//				what we've got
			// ____________________________________
			// |								  |
			//	 [us]	[ms]  	[s]		[m]		[h]
			//	  |_____________| |______________|		
			//		 discard		what we want	

			// Example #1:
			// this		=  		[ ms=0, s=0 ]
			// other	= [ us=5, ms=9, s=2, min=1  ]			
			// by       =       [ ms=?, s=? ]				= intersect(this, other)
			// this_res	=  		[ ms=9, s=2 ]

			// Example #2:
			// this		= [ us=0, ms=0, s=5, min=10 ]
			// other	=		      [ s=5, min=10, h=3 ]
			// by       =             [ s=?, min=?  ]		= intersect(this, other)
			// this_res	= [ us=0, ms=0, s=5, min=10, h=0 ]

			copy_tuple_by_types<intersect_ordered_tuples_t<decltype(_durations), decltype(other._durations)>>(_durations, other._durations);

			if constexpr (std::ratio_less_v<typename high_t::period, typename H::period>)
			{
				// conversion that converts greater units into smaller ones and then discards those greater units
				// 
				//				what we've got
				// ____________________________________
				// |								  |
				//	 [us]	[ms]  	[s]		[m]		[h]
				//			 ^_____/ ^______/ ^______/
				//				+=		+=		 +=
				// |______________|		
				//  what we want		

				// Example #1:
				// this		=  		[ ms=9, s=2 ]
				// other	= [ us=5, ms=9, s=2, min=1  ]			
				// other_truncated	=	  [ s=2, min=1  ] -> seconds(62)		
				// this_res	=  		[ ms=9, s=62 ]

				// Example #2:
				// this		= [ us=0, ms=0, s=5, min=10 ]
				// other	=		      [ s=5, min=10, h=3 ]
				// other_truncated =			[min=10, h=3 ] -> 190
				// this_res	= [ us=0, ms=0, s=5, min=10 + 190 = 200 ]
				const Time<high_t, H> other_truncated(other);
				std::get<high_t>(_durations) = static_cast<high_t>(other_truncated);
			}
		}
		else
		{
			copy_tuple_by_types(_durations, other._durations, other._durations);
		}

		normalize(*this);
		return;
	}

	template <typename Duration>
	constexpr explicit operator Duration() const
	{
		Duration result(0);
		for_each(_durations, [&result](auto&& elem)
			{ result += std::chrono::duration_cast<Duration>(elem); });
		return result;
	}

	template <typename Duration>
	constexpr const Duration& get() const { return std::get<Duration>(_durations); }

	template <typename Duration>
	Duration& get() { return std::get<Duration>(_durations); }

	// TODO: SET()

	constexpr Time operator+ (const Time& other) const
	{
		Time result = *this;
		apply_and_normalize(result, [&other](auto&& current_dur_v, auto idx)
			{ current_dur_v += std::get<idx>(other._durations); });
		return result;
	}

	template <typename L, typename H>
	constexpr auto operator+(const Time<L, H>& other) const
	{
		using	BroadenedTime = Time<low_broader_t<low_t, L>, high_broader_t<high_t, H>>;
		return	BroadenedTime(*this) + BroadenedTime(other);
	}

	// TODO: TOP PRIORITY, impl operator-(Time, Time)

	constexpr Time operator- (const Time& other) const
	{
		Time result = *this;
		apply_and_normalize(result, [&other](auto&& current_dur_v, auto idx)
			{ current_dur_v -= std::get<idx>(other._durations); });
		return result;
	}

	template <typename L, typename H>
	constexpr auto operator-(const Time<L, H>& other) const
	{
		using	BroadenedTime = Time<low_broader_t<low_t, L>, high_broader_t<high_t, H>>;
		return	BroadenedTime(*this) - BroadenedTime(other);
	}

	friend constexpr std::ostream& operator<<(std::ostream& os, const Time<low_t, high_t>& time)
	{
		os << "[ ";
		for_each(time._durations, [&os](auto&& f) { os << f.count() << ' ' << duration_name_v<decltype(f)> << "; "; });
		os << ']';
		return os;
	}
};

using DefaultTime = Time<std::chrono::seconds, std::chrono::hours>;

// Returns current time
static DefaultTime now()
{
	using namespace std::chrono;
	auto current	= system_clock::now();

	struct tm time;
	auto now_time_t = system_clock::to_time_t(current);
	(void)localtime_s(&time, &now_time_t);

	return { static_cast<seconds>(time.tm_sec), static_cast<minutes>(time.tm_min), static_cast<hours>(time.tm_hour) };
}


template<typename... Durations> // 0 or more durations
Time(const Durations&... durations) ->Time<std::chrono::seconds>;

template<typename DurationLow> // 1 duration
Time(const DurationLow&) -> Time<DurationLow>;

template<typename DurationLow, typename Duration, typename... Durations> // 2 or more durations
Time(const DurationLow&, const Duration&, const Durations&...) -> Time<DurationLow, ith_type_t<sizeof...(Durations), Duration, Durations...>>;

