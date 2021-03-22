#pragma once
#include <chrono>
#include <ostream>
#include <algorithm>
#include "BitUtils.h"

namespace btime
{
	template <class ChronoDuration>
	constexpr void max_time_value()
	{
		return std::max(ChronoDuration::period::num, ChronoDuration::period::den);
	}

	template <class ChronoDuration>
	using min_suitable_time_type = typename min_suitable_uint_type<
		bytecount<max_time_value<ChronoDuration>>::value
	>::type;

	using hours_t	= min_suitable_time_type<std::chrono::hours>;
	using minutes_t = min_suitable_time_type<std::chrono::minutes>;
	using seconds_t = min_suitable_time_type<std::chrono::seconds>;

	static constexpr hours_t	max_hours	= max_time_value<hours_t>();
	static constexpr minutes_t	max_minutes = max_time_value<minutes_t>();
	static constexpr seconds_t	max_seconds = max_time_value<seconds_t>();


	class Time
	{
	private:

		hours_t		_hours;
		minutes_t	_minutes;
		seconds_t	_seconds;

		template <std::chrono::duration dur>
		constexpr bool isValid() const noexcept
		{
			return d <= max_time_value<decltype(d)>();
		}

		void normalize() noexcept
		{
			_hours	+= _minutes / max_minutes + _seconds / max_hours;
			_minutes = _minutes % max_minutes + _seconds / max_minutes % max_minutes;
			_seconds %= max_minutes;
		};

	public:

		Time()
		{
			struct tm today;
			time_t t	= time(NULL);
			localtime_s(&today, &t);
			_minutes	= static_cast<decltype(_minutes)>(today.tm_min);
			_seconds	= static_cast<decltype(_seconds)>(today.tm_sec);
			_hours		= static_cast<decltype(_hours)>(today.tm_hour);
		}

		Time(decltype(_seconds) seconds, decltype(_minutes) minutes, decltype(_hours) hours) :
			_minutes(minutes),
			_seconds(seconds),
			_hours(hours)
		{
			normalize();
		}

		inline	seconds_t	seconds()	const noexcept { return _seconds;	}
		inline	hours_t		hours()		const noexcept { return _hours;		}
		inline	minutes_t	minutes()	const noexcept { return _minutes;	}

		seconds_t seconds(seconds_t s)
		{
			if (s > 0) _seconds = s;
			normalize();
			return _seconds;
		}

		hours_t hours(hours_t h)
		{
			if (h > 0) _hours = h;
			normalize();
			return _hours;
		}

		minutes_t minutes(minutes_t m)
		{
			if (m > 0) _minutes = m;
			normalize();
			return _minutes;
		}

		bool operator<(const Time t)const {
			if (_hours != t._hours)return _hours < t._hours;
			if (_minutes != t._minutes)return _minutes < t._minutes;
			if (_seconds != t._seconds)return _seconds < t._seconds;
		}
		const Time operator+(const Time& t)const {

			short int s = (_seconds + t._seconds);
			short int m = (_minutes + t._minutes) + s / 60;
			short int h = (_hours + t._hours) + m / 60;

			return Time(s % 60, m % 60, h % 24);
		}

		bool operator==(const Time& t)const {
			return _minutes == t._minutes && _seconds == t._seconds && _hours == t._hours;
		}

		long toSeconds()const {
			return _minutes * 60 + _seconds + _hours * 3600;
		}
		static long day() {
			return 24 * 3600;
		}
		Time& operator=(const Time& other) {
			_hours = other._hours;
			_minutes = other._minutes;
			_seconds = other._seconds;
			return *this;
		}

	};
	std::ostream& operator<<(std::ostream& os, const Time& t) {
		os << t.hours() << ":" << t.minutes() << ":" << t.seconds() << "\n";
		return os;
	}
}

