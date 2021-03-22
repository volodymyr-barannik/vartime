#include <iostream>
//#include "Time.h"
#include "TrueTime.h"
#include "Algorithm.h"
#include "BitUtils.h"
using std::cout;
using std::endl;
using namespace std::chrono;

template <class T>
constexpr void test_helper(T&&) {}

#define IS_CONSTEXPR(...) noexcept(test_helper(__VA_ARGS__))

static constexpr char nendl = '\n';

int main() {
	using namespace std::literals::chrono_literals;
	using namespace std::chrono;

	using Tm = Time<microseconds, minutes>;

	constexpr Tm t0{ };
	
	constexpr auto t0c = IS_CONSTEXPR(Tm{ });
	constexpr auto t1c = IS_CONSTEXPR(Tm{ {10us,  20ms,  0s,  5min} });
	constexpr auto t2c = IS_CONSTEXPR(Tm{ } + Tm{ {10us,  20ms,  0s,  5min} });
	constexpr auto t3c = IS_CONSTEXPR(seconds{5s});
	std::cout << t0c << t1c << t2c << t3c << nendl;

	std::cout << "t0: " << t0 << nendl;
	constexpr Tm t1{ {10us,  20ms,  0s,  5min}  };
	std::cout << "t1: " << t1 << nendl;
	constexpr Tm t2{ {990us, 980ms, 20s, 10min} };
	std::cout << "t2: " << t2 << nendl;
	constexpr Time<seconds, hours> t3{ {20s, 10min, 14h} };
	std::cout << "t3: " << t3 << nendl;

	// THESE DO NOT WORK AS WELL :(
	constexpr auto r1microseconds = t1.get<microseconds>();
	std::cout << "microseconds of t1 before change: " << r1microseconds << '\n';
	//t1.set<microseconds>(71us);
	std::cout << "microseconds of t1 after change: " << t1.get<microseconds>() << '\n';
	//t1.set<microseconds>(1200us);
	//std::cout << "microseconds of t1 after change: " << t1.get<microseconds>() << '\n';
	//std::cout << "t1 after change: " << t1 << '\n';
	
	constexpr auto r1 = t1 + t2;
	constexpr auto r2 = t1 + t3;
	//constexpr auto r3 = r2 + t2;
	std::cout << "t1 + t2 = ";
	std::cout << r1 << nendl;
	//std::cout << "t1 + t3 = ";
	//std::cout << r2 << nendl;
	//std::cout << "r2 + t2 = ";
	//std::cout << r3 << nendl;
	//std::cout << r2;
	//print(std::cout, r);
}
