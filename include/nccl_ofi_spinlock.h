/*
 * Copyright (c) 2026      Amazon.com, Inc. or its affiliates. All rights reserved.
 */

#ifndef NET_OFI_SPINLOCK_H_
#define NET_OFI_SPINLOCK_H_

#include <atomic>


// A BasicLockable spinlock without many features
class CAPABILITY("mutex") nccl_ofi_spinlock {
public	:
	nccl_ofi_spinlock() : val(UNLOCKED), timer(nullptr)
	{
		std::atomic_thread_fence(std::memory_order_release);
	}

	nccl_ofi_spinlock(const nccl_ofi_spinlock&) = delete;
	nccl_ofi_spinlock& operator=(const nccl_ofi_spinlock&) = delete;

	bool trylock() TRY_ACQUIRE(true)
	{
		lock_type::value_type old = UNLOCKED;
		bool ret = val.compare_exchange_strong(old, LOCKED, std::memory_order_seq_cst);
		return !ret;
	}


	void lock() ACQUIRE()
	{
		while (this->trylock()) {
			while (val.load() == LOCKED) {
#if defined(__x86_64__)
				asm volatile("pause" : : : );
#elif defined(__aarch64__)
				asm volatile("isb" : : : );
#endif
			}
		}
	}


	void unlock() ACQUIRE()
	{
#if(PROF_ISEND & PROF_MUTEX)
		if (timer && time_unlocks) {
			timer->start_timer();   // Dec 09 narrow down 270ns
		}
#endif
		val.store(UNLOCKED, std::memory_order_release);
#if(PROF_ISEND & PROF_MUTEX)
		if (timer && time_unlocks) {
			timer->stop_timer();   // Dec 09 narrow down 270ns
			time_unlocks = false;
			timer = nullptr;
		}
#endif

	}

	void set_time_unlocks(bool value)
	{
		time_unlocks = value;
	}

	void set_timer(timer_histogram<histogram_custom_binner<size_t> > *_timer) {
		timer = _timer;
	}

private:
	using lock_type = std::atomic<int>;
	enum state { UNLOCKED = 0, LOCKED = 1};

	lock_type val;
	timer_histogram<histogram_custom_binner<size_t> > *timer;
	bool time_unlocks;
};

#endif