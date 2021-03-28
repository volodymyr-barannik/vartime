#pragma once
#include <thread>
#include "Time.h"

template <typename Duration>
class Timer
{
private:
	std::thread _thread;

public:

	template<typename L, typename H, typename Functor, typename... Args>
	explicit Timer(Time<L, H>&& time, const bool sync, Functor&& fn, Args&&... args)
	{
		const Duration duration = static_cast<Duration>(time);
		if (sync)
		{
			std::this_thread::sleep_for(duration);
			std::invoke(fn, std::forward<Args>(args)...);
		}
		else
		{
			_thread = std::thread([duration, &fn, &args...] // We're passing duration by value because duration is a local variable.
															// Thread might start its work after we left the constructor.
															// Thus, we might get a reference to destroyed object
				{
					std::this_thread::sleep_for(duration);
					std::invoke(fn, std::forward<decltype(args)>(args)...);
				});
			_thread.detach();
		}
	}

	bool elapsed() const { return _thread.joinable(); }
};

template<typename L, typename H, typename Functor, typename... Args>
Timer(Time<L, H>&&, const bool, Functor&&, Args&&...) -> Timer<L>;

