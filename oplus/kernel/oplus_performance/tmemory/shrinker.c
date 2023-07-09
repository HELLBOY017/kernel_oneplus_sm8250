#include "tmemory.h"

#include <linux/fs.h>

static LIST_HEAD(tmemory_list);
static DEFINE_SPINLOCK(tmemory_list_lock);
static unsigned int shrinker_run_no;

unsigned long tmemory_shrink_count(struct shrinker *shrink,
				struct shrink_control *sc)
{
	struct tmemory_device *tm;
	struct list_head *p;
	unsigned long count = 0;

	spin_lock(&tmemory_list_lock);
	p = tmemory_list.next;
	while (p != &tmemory_list) {
		tm = list_entry(p, struct tmemory_device, shrink_list);

		spin_unlock(&tmemory_list_lock);

		/* count free nids cache entries */
		count += atomic_read(&tm->dirty_pages);

		spin_lock(&tmemory_list_lock);
		p = p->next;
	}
	spin_unlock(&tmemory_list_lock);
	return count;
}

unsigned long tmemory_shrink_scan(struct shrinker *shrink,
				struct shrink_control *sc)
{
	unsigned long nr = sc->nr_to_scan;
	struct tmemory_device *tm;
	struct list_head *p;
	unsigned int run_no;
	unsigned long freed = 0;

	spin_lock(&tmemory_list_lock);
	do {
		run_no = ++shrinker_run_no;
	} while (run_no == 0);

	p = tmemory_list.next;

	while (p != &tmemory_list) {
		tm = list_entry(p, struct tmemory_device, shrink_list);

		if (tm->shrinkno == run_no)
			break;

		spin_unlock(&tmemory_list_lock);

		tm->shrinkno = run_no;

		tm->need_commit = true;
		wake_up_interruptible_all(&tm->commit_trans_wait);

		spin_lock(&tmemory_list_lock);
		p = p->next;
		list_move_tail(&tm->shrink_list, &tmemory_list);
		if (freed >= nr)
			break;
	}
	spin_unlock(&tmemory_list_lock);
	return freed;
}

void tmemory_join_shrinker(struct tmemory_device *tm)
{
	spin_lock(&tmemory_list_lock);
	list_add_tail(&tm->shrink_list, &tmemory_list);
	spin_unlock(&tmemory_list_lock);
}

void tmemory_leave_shrinker(struct tmemory_device *tm)
{
	spin_lock(&tmemory_list_lock);
	list_del_init(&tm->shrink_list);
	spin_unlock(&tmemory_list_lock);
}
