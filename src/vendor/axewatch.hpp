/**
 * MIT License
 * Copyright (c) 2020 Carlos Machado
 * https://github.com/carlosebmachado/watch
 * v1.3.15
 */

#ifndef AXEWATCH_HPP
#define AXEWATCH_HPP

#include <time.h>
#include <chrono>
#include <string>
#include <sstream>

namespace axe {

#define MILLISECOND 1
#define SECOND (1000 * MILLISECOND)
#define MINUTE (60 * SECOND)
#define HOUR (60 * MINUTE)
#define DAY (24 * HOUR)

#define CHR_NANOSECOND 1
#define CHR_MICROSECOND (1000 * CHR_NANOSECOND)
#define CHR_MILLISECOND (1000 * CHR_MICROSECOND)
#define CHR_SECOND (1000 * CHR_MILLISECOND)

	typedef int64_t watch_t;


	class Watch {
	protected:
		watch_t elapsed;

	public:
		// constructors
		Watch(watch_t elapsed) : elapsed(elapsed) {}

		watch_t get_elapsed() {
			return elapsed;
		}

		// control flow
		virtual void start() = 0;

		virtual void stop() = 0;

		virtual void reset() = 0;

		void restart() {
			reset();
			start();
		}

		// getters and setters
		virtual std::string get_elapsed_formatted() = 0;
	};

	class Stopwatch : public Watch {
	private:
		watch_t start_time;

	public:
		// constructors
		Stopwatch() : start_time(0), Watch(0) { }

		~Stopwatch() = default;

		// control flow
		void start() override {
			start_time = clock();
		}

		void stop() override {
			elapsed = clock() - start_time;
		}

		void reset() override {
			elapsed = 0;
			start_time = 0;
		}

		// getters and setters
		std::string get_elapsed_formatted() override {
			watch_t d = 0, h = 0, m = 0, s = 0, mil = 0, aux_elapsed = elapsed;
			d = aux_elapsed / DAY;
			aux_elapsed %= DAY;
			h = aux_elapsed / HOUR;
			aux_elapsed %= HOUR;
			m = aux_elapsed / MINUTE;
			aux_elapsed %= MINUTE;
			s = aux_elapsed / SECOND;
			aux_elapsed %= SECOND;
			mil = aux_elapsed;
			std::stringstream r;
			if (d) {
				r << d << "d ";
			}
			if (h) {
				r << h << "h ";
			}
			if (m) {
				r << m << "m ";
			}
			if (s) {
				r << s << "s ";
			}
			if (mil) {
				r << mil << "mil";
			}
			return r.str();
		}
	};


	class ChronoStopwatch : public Watch {
	private:
		std::chrono::steady_clock::time_point start_time;

	public:
		// constructors
		ChronoStopwatch() : start_time(std::chrono::high_resolution_clock::now()), Watch(0) { }

		~ChronoStopwatch() = default;

		// control flow
		void start() override {
			start_time = std::chrono::high_resolution_clock::now();
		}

		void stop() override {
			elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - start_time).count();
		}

		void reset() override {
			elapsed = 0;
			start_time = std::chrono::high_resolution_clock::now();
		}

		// getters and setters
		std::string get_elapsed_formatted() override {
			watch_t min = 0, sec = 0, mil = 0, mic = 0, nan = 0, aux_elapsed = elapsed;
			sec = aux_elapsed / CHR_SECOND;
			aux_elapsed %= CHR_SECOND;
			mil = aux_elapsed / CHR_MILLISECOND;
			aux_elapsed %= CHR_MILLISECOND;
			mic = aux_elapsed / CHR_MICROSECOND;
			aux_elapsed %= CHR_MICROSECOND;
			nan = aux_elapsed;
			while (sec >= 60) {
				sec -= 60;
				min++;
			}
			std::stringstream r;
			if (min) {
				r << min << "min ";
			}
			if (sec) {
				r << sec << "sec ";
			}
			if (mil) {
				r << mil << "mil ";
			}
			if (mic) {
				r << mic << "mic ";
			}
			if (nan) {
				r << nan << "nan ";
			}
			return r.str();
		}
	};


	class Timer {
	private:
		clock_t start_time;
		clock_t interval;

	public:
		// constructors
		Timer() : start_time(0), interval(0) { }

		Timer(clock_t interval) : start_time(0), interval(interval) { }

		~Timer() = default;

		// control flow
		void start() {
			start_time = clock();
		}

		bool timeout() {
			if (clock() - start_time > interval) {
				start_time = clock();
				return true;
			}
			return false;
		}

		// getters and setters
		void set_interval(clock_t interval) {
			this->interval = interval;
		}

		clock_t get_interval() {
			return interval;
		}
	};

}

#endif // !AXEWATCH_HPP
