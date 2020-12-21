/*
 * scan.h  nl80211 scanning thread using my libiw
 *
 * Copyright (c) 2019-2020 David Poole <davep@mbuf.com>
 */

#ifndef SCAN_H
#define SCAN_H

#include <vector>
#include <thread>
#include <atomic>

struct iw_dev;

class ScanningThread
{
	public:
		ScanningThread();
		~ScanningThread();

		void stop(void);

		void join(void) { thd.join(); };

		static void run(ScanningThread* self) { 
			self->init_worker();
			self->run_worker();
//			while(not self->stop_flag) {
//				std::cout << "hello " << self->get_counter() << " from thread\n" ;
//				std::this_thread::sleep_for(std::chrono::seconds(1));
//			};
		};

	private:
		std::thread thd;

		int init_worker(void);
		int run_worker(void);

		class ScanningInternal;
		std::unique_ptr<ScanningInternal> worker;
};

#endif

