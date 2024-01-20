#include "tmemory.h"

const char *tmemory_fault_name[FAULT_MAX] = {
	[FAULT_PAGE_ALLOC]	= "page alloc",
	[FAULT_READ_IO] 	= "read IO error",
	[FAULT_ISSUE_DISCARD] 	= "issue discard error",
	[FAULT_COMPLETE_DISCARD]= "complete discard error",
	[FAULT_WRITE_IO]	= "write IO error",
	[FAULT_SLAB_ALLOC]	= "slab alloc",
	[FAULT_RADIX_INSERT]	= "radix_insert",
	[FAULT_FLUSH]		= "flush",
	[FAULT_DELAY]		= "delay",

	[FAULT_PANIC_STATE_LOCK]	= "panic in tm->state_lock",
	[FAULT_PANIC_DISCARD_LOCK]	= "panic in tm->discard_lock",
	[FAULT_PANIC_UPDATE_LOCK]	= "panic in tm->update_lock",
	[FAULT_PANIC_MIGRATE_LOCK]	= "panic in tm->migrate_lock",
	[FAULT_PANIC_COMMIT_RWSEM]	= "panic in tm->commit_rwsem",
	[FAULT_PANIC_NOMEM_LOCK]	= "panic in tm->nomem_lock",
	[FAULT_PANIC_PAGECRYPT_LOCK]	= "panic in page->crypt_lock",
	[FAULT_PANIC_COMMIT_LOCK]	= "panic in tm->commit_lock",
	[FAULT_PANIC_WENDIO]		= "panic in write endio",
	[FAULT_PANIC_RENDIO]		= "panic in read endio",
};

#define TMEMORY_ALL_FAULT_TYPE		((1 << FAULT_MAX) - 1)

#define IS_FAULT_SET(ifi, type) ((ifi)->inject_type & (1 << (type)))

void tmemory_build_fault_attr(struct tmemory_device *tm, unsigned int rate,
							unsigned int type)
{
	struct tmemory_fault_info *ifi = &tm->fault_info;

	if (rate) {
		atomic_set(&ifi->inject_ops, 0);
		ifi->inject_rate = rate;
	}

	if (type)
		ifi->inject_type = type;

	if (!rate && !type) {
		memset(ifi, 0, sizeof(struct tmemory_fault_info));
		atomic_set(&ifi->inject_ops, 0);
	}
}

bool time_to_inject(struct tmemory_device *tm, int type)
{
	struct tmemory_fault_info *ifi = &tm->fault_info;

	if (!ifi->inject_rate)
		return false;

	if (!IS_FAULT_SET(ifi, type))
		return false;

	atomic_inc(&ifi->inject_ops);
	if (atomic_read(&ifi->inject_ops) >= ifi->inject_rate) {
		atomic_set(&ifi->inject_ops, 0);
		return true;
	}
	return false;
}
