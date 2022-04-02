#pragma once
#ifndef STOPWATCH_H_INCLUDED
#define STOPWATCH_H_INCLUDED

#include <chrono> // steady_clock, time_point, ...
#include <vector>
#include <stdexcept>
#include <string_view>
#include <sstream>
#include <ostream>

/*
	This utility conforms to the Council of Ricks standard.
	As well as the usual Future Gadget Laboratory standard.

	This utility is not TARDIS or DeLorean safe;
	start must be called before stop.

	Writen in worldline Divergence 1.048596 by Alaestor.
	Discord Honshitsu#9218
*/

namespace stopwatch {
class Stopwatch
{
	typedef std::chrono::steady_clock clock;

public:
	const std::string m_name;
	const std::chrono::time_point<clock> m_unset{};
	std::vector<std::chrono::time_point<clock>> m_laps{};
	std::chrono::time_point<clock> m_start{}, m_stop{};

	[[nodiscard]]
	static std::string formatted(std::chrono::nanoseconds elapsed)
	{
		std::ostringstream os;
		using // using namespace std::chrono doesn't include nanoseconds?
			std::chrono::duration_cast,
			std::chrono::nanoseconds,
			std::chrono::microseconds,
			std::chrono::milliseconds,
			std::chrono::seconds,
			std::chrono::minutes,
			std::chrono::hours;

		auto remaining{ elapsed };

		auto process{
			[&remaining, &os]<typename T>(T time, std::string_view abbrev)
			{
				remaining -= time;
				if (const auto& t{ time.count() }; t > 0)
					os << t << abbrev << " ";
			}
		};

		process(duration_cast<hours>(remaining), "h");
		process(duration_cast<minutes>(remaining), "m");
		process(duration_cast<seconds>(remaining), "s");
		process(duration_cast<milliseconds>(remaining), "ms");
		process(duration_cast<microseconds>(remaining), "us");
		process(duration_cast<nanoseconds>(remaining), "ns");

		if (remaining.count() > 0)
			os << "\n... wtf? Something probably broke.\n";

		return os.str();
	}

public:
	void start()
	{ m_start = clock::now(); }

	void stop()
	{ m_stop = clock::now(); }

	void reset()
	{ m_start = m_stop = m_unset; }

	[[nodiscard]]
	std::string_view getName() const
	{ return m_name; }

	void lap()
	{
		if (m_start == m_unset)
			throw std::runtime_error(m_name+" must start before you can lap!");

		m_laps.push_back(clock::now());
	}

	void clearLaps()
	{ m_laps.clear(); }

	[[nodiscard]] std::size_t numberOfLaps() const noexcept
	{ return m_laps.size(); }

	[[nodiscard]] std::string getLap(const std::size_t number) const
	{
		if (m_laps.size() == 0)
			throw std::runtime_error(m_name+" has no laps");

		if (number == 0)
			throw std::invalid_argument(m_name+" getLap number must be > 0");

		const std::size_t i{ number-1 };

		if (i > m_laps.size())
			throw std::invalid_argument(m_name+" getLap number out of range");

		const auto lap{ m_laps.at(i) };
		const auto lastLap{ i > 1 ? m_laps.at(i-1) : m_start };
		std::stringstream sstream;
		sstream
			<< m_name << " lap " << number << ": "
			<< formatted(lap - lastLap);

		return sstream.str();
	}

	[[nodiscard]] std::string previousLap() const
	{ return getLap(m_laps.size()); }

	[[nodiscard]] std::string allLaps() const
	{
		std::stringstream sstream;

		auto lastLap{ m_start };
		std::size_t counter{ 1 };
		for (const auto& lap : m_laps)
		{
			sstream
				<< m_name << " Lap " << counter << ": "
				<< formatted(lap - lastLap) << "\n";
			lastLap = lap;
			++counter;
		}

		return sstream.str();
	}

	[[nodiscard]] std::string averageLaps() const
	{
		if (m_laps.size() == 0)
			throw std::runtime_error(m_name+" has no laps");

		std::chrono::nanoseconds accum{ std::chrono::nanoseconds::zero() };
		auto lastLap{ m_start };
		for (const auto& lap : m_laps)
		{
			accum += lap - lastLap;
			lastLap = lap;
		}

		const auto avg{ accum / m_laps.size() };

		return formatted(avg);
	}

	friend std::ostream& operator<<(std::ostream& os, const Stopwatch& sw)
	{
		return os
			<< sw.m_name << ": "
			<< sw.formatted(sw.m_stop - sw.m_start);
	}

	explicit Stopwatch(std::string_view name)
	: m_name(name)
	{
		m_laps.reserve(10000);
		static_assert(clock::is_steady,
			"stopwatch chrono::steady_clock is not steady; not OS supported?");
	}

	~Stopwatch() = default;
};
}// namespace stopwatch

#endif // STOPWATCH_H_INCLUDED
