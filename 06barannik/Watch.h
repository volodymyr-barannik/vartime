#pragma once
#include "Timer.h"

template <typename Duration>
class Watch
{
private:

	Timer<Duration> _timer;

public:

	template<typename L, typename H, typename Functor, typename... Args>
	explicit Watch(Time<L, H>&& time, const bool sync, Functor&& fn, Args&&... args) :
		_timer(std::forward<Time<L, H>>(time - now()), sync,
			   std::forward<decltype(fn)>(fn), std::forward<decltype(args)>(args)...) {}

	bool elapsed() const { return _timer.elapsed(); }
};


template<typename L, typename H, typename Functor, typename... Args>
Watch(Time<L, H>&&, const bool, Functor&&, Args&&...) -> Watch<L>;