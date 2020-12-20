/*
 * scan.h  nl80211 scanning thread using my libiw
 *
 * Copyright (c) 2019-2020 David Poole <davep@mbuf.com>
 */

#ifndef SCAN_H
#define SCAN_H

class ScanningThread
{
	public:
		ScanningThread() : thd(run, this),
							stop_flag(false),
							counter(0)
		{ 
		};

		void stop(void) { 
			stop_flag = true;
		};

		void join(void) { thd.join(); };

		uint64_t get_counter(void) {
			return this->counter++;
		};

		static void run(ScanningThread* self) { 
			while(not self->stop_flag) {
				std::cout << "hello " << self->get_counter() << " from thread\n" ;
				std::this_thread::sleep_for(std::chrono::seconds(1));
			};
		};

	private:
		std::thread thd;
		std::atomic<bool> stop_flag;

		uint64_t counter;
};

#endif

