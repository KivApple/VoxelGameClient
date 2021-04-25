#pragma once

#include <iostream>
#include <chrono>
#include <shared_mutex>

class PerformanceCounter {
	long long m_last = 0;
	long long m_min = 0;
	long long m_max = 0;
	long long m_sum = 0;
	int m_count = 0;
	std::shared_mutex m_mutex;
	
	friend inline std::ostream &operator<<(std::ostream &os, PerformanceCounter &counter);
	
public:
	void reset() {
		std::unique_lock<std::shared_mutex> lock(m_mutex);
		m_count = 0;
	}
	
	void update(long long value) {
		std::unique_lock<std::shared_mutex> lock(m_mutex);
		m_last = value;
		if (m_count > 0) {
			m_sum += value;
			if (value < m_min) {
				m_min = value;
			}
			if (value > m_max) {
				m_max = value;
			}
			m_count++;
		} else {
			m_sum = value;
			m_min = value;
			m_max = value;
			m_count = 1;
		}
	}
	
};

class PerformanceMeasurement {
	PerformanceCounter *m_counter;
	std::chrono::steady_clock::time_point m_start;
	
public:
	explicit PerformanceMeasurement(PerformanceCounter &counter): m_counter(&counter) {
		m_start = std::chrono::steady_clock::now();
	}
	
	PerformanceMeasurement(
			PerformanceMeasurement &&measurement
	) noexcept: m_counter(measurement.m_counter), m_start(measurement.m_start) {
		measurement.m_counter = nullptr;
	}
	
	PerformanceMeasurement &operator=(PerformanceMeasurement &&measurement) noexcept {
		m_counter = measurement.m_counter;
		m_start = measurement.m_start;
		measurement.m_counter = nullptr;
		return *this;
	}
	
	~PerformanceMeasurement() {
		finish();
	}
	
	void finish() {
		if (m_counter != nullptr) {
			m_counter->update(std::chrono::duration_cast<std::chrono::milliseconds>(
					std::chrono::steady_clock::now() - m_start
			).count());
			m_counter = nullptr;
		}
	}
	
	void discard() {
		m_counter = nullptr;
	}
	
};

inline std::ostream &operator<<(std::ostream &os, PerformanceCounter &counter) {
	std::shared_lock<std::shared_mutex> lock(counter.m_mutex);
	auto count = counter.m_count;
	auto last = counter.m_last;
	auto sum = counter.m_sum;
	auto min = counter.m_min;
	auto max = counter.m_max;
	lock.unlock();
	if (count > 0) {
		os << "min=" << min << ",avg=" << (sum / count) << ",max=" << max << ",last=" << last;
		os << " (" << count << " samples)";
	} else {
		os << "no data";
	}
	return os;
}
