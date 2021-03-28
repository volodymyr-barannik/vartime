#include <iostream>
#include "Time.h"
//#include "Watch.h"
#include "Timer.h"
using namespace std::literals::chrono_literals;
using namespace std::chrono;
using std::cout;
using std::endl;
static constexpr char nendl = '\n';

namespace tests
{
	namespace time
	{
		void test()
		{
			using Tm = Time<microseconds, minutes>;

			auto								t_now(now());
			//Time<microseconds, minutes>			t_now2(now());
			constexpr Time						t0{ };
			constexpr Tm						t1{ 10us,  20ms,  0s,  1min };
			constexpr Time<microseconds, hours> t2{ 990us, 980ms, 20s, 10min, 2h };
			constexpr Time						t3{ 20s, 10min, 14h };
			//constexpr Time						t_wrong{ 20s, 14h }; // minutes are missing here
			constexpr Time<seconds, hours>		t3copy(t3);
					  Time<seconds, hours>		t3copy_non_constexpr(t3);
			constexpr Time						t3copy_type_deduced(t3);
			constexpr Time<minutes, hours>		t3truncated(t3);
			constexpr Time<seconds, minutes>	t3lowered(t3);
			Time<seconds, minutes>	t2truncated_and_lowered(t2);

			cout << "t_now:\t\t\t" << t_now << nendl;
			//cout << "t_now2:\t\t\t" << t_now2 << nendl;
			cout << "t0:\t\t\t" << t0 << nendl;
			cout << "t1:\t\t\t" << t1 << nendl;
			cout << "t2:\t\t\t" << t2 << nendl;
			cout << "t3:\t\t\t" << t3 << nendl;
			cout << "t3copy:\t\t\t" << t3copy << nendl;
			cout << "t3copy_non_constexpr:\t" << t3copy_non_constexpr << nendl;
			cout << "t3copy_type_deduced:\t" << t3copy_type_deduced << nendl;
			cout << "t3truncated:\t\t" << t3truncated << nendl;
			cout << "t3lowered:\t\t" << t3lowered << nendl;
			cout << "t2truncated_and_lowered:" << t2truncated_and_lowered << nendl;

			constexpr auto t1microseconds = t1.get<microseconds>();
			cout << "microseconds of t1:\t" << t1microseconds << nendl;

			constexpr seconds t1_in_seconds = static_cast<seconds>(t1);
			cout << "t1 in seconds:\t\t" << t1_in_seconds << nendl;

			cout << nendl;

			// You might get an IntelliSense warning here. Probably, it's a bug.
			// I've tried both gcc and clang -- no problems there, compiles perfectly
			constexpr auto r1 = t1 + t2;
			constexpr auto r2 = t1 + t3;
			constexpr auto r3 = r2 + t2;
			cout << "r1 = t1 + t2 =\t\t" << r1 << nendl;
			cout << "r2 = t1 + t3 =\t\t" << r2 << nendl;
			cout << "r3 = r2 + t2 =\t\t" << r3 << nendl;
		}
	}

	namespace timer
	{
		void test()
		{
			Timer<seconds>			timer1(Time{ 2s },		false,	[]() {cout << "#1\tTimer<seconds>\t\t[async]\t(2s)\t\tis done!" << nendl; });
			Timer					timer2(Time{ 1s },		false,	[]() {cout << "#2\tTimer\t\t\t[async]\t(1s)\t\tis done!" << nendl; });
			Timer<milliseconds>		timer3(Time{ 25ms, 1s },false,	[]() {cout << "#3\tTimer<milliseconds>\t[async]\t(25ms, 1s)\tis done!" << nendl; });
			Timer<milliseconds>		timer4(Time{},			false,	[]() {cout << "#4\tTimer<milliseconds>\t[async]\t(0s)\t\tis done!" << nendl; });
			Timer<hours>			timer5(Time{ 10s },		true,	[]() {cout << "#5\tTimer<hours>\t\t[async]\t(10s)\t\tis done!" << nendl; });
			Timer<milliseconds>		timer6(Time{ 4s },		true,	[]() {cout << "#6\tTimer<milliseconds>\t[sync]\t(10s)\t\tis done!" << nendl; });
		}
	}
}


int main() {
	cout << "--------------Testing Time class--------------" << nendl;
	tests::time::test();
	//constexpr Time t1{ 10us,  20ms,  0s,  1min };
	std::cout << nendl << "--------------Testing Timer class--------------" << nendl;
	//tests::timer::test();
	std::cout << nendl;
	
	std::cout << "END" << std::endl;
}
