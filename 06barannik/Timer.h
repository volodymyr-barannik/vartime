#pragma once
#include <thread>
#include "Time.h"

// Timer that has a duration of type Duration.
// Starts immediately after its creation.
template <typename Duration>
class Timer
{
private:

	std::thread _thread;
	bool _elapsed = false;

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
			// We're passing duration by value because duration is a local variable.
			// Thread might start its execution after we left the constructor.
			// Thus, we might get a reference to destroyed object :(
			_thread = std::thread([this, duration, &fn, &args...]	
				{
					std::this_thread::sleep_for(duration);
					std::invoke(fn, std::forward<decltype(args)>(args)...);
					_elapsed = true;
				});
			_thread.detach();
		}
	}

	bool elapsed() const { return _elapsed; }
};

template<typename L, typename H, typename Functor, typename... Args>
Timer(Time<L, H>&&, const bool, Functor&&, Args&&...) -> Timer<L>;

