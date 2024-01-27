#include <linux/sched.h>
#include <linux/ctype.h>
#ifdef CONFIG_OPLUS_FEATURE_IM
#include <linux/im/im.h>
#endif

#include <../kernel/sched/walt/qc_vas.h>
#include "webview_boost.h"

#define TOPAPP 4
#define MIN_BOOST_VLAUE 15
#define CHILD_MAX 10

static int webview_debug = 0;
module_param_named(debug,webview_debug,uint,0644);
static struct web_target {
       char val[TASK_COMM_LEN + 1];
       char* desc[CHILD_MAX];
} web_target [] = {
       {"cent.mm:toolsmp", {"Chrome_InProc", "Compositor"}},
       {"m.taobao.taobao", {"Chrome_IOThread", "Chrome_ChildIOT", "Chrome_InProc"}},
       {"vilege_process0", {"Chrome_ChildIOT", "Compositor", "CrRendererMain"}},
       {"one:gpu_process", {"CrGpuMain"}},
       {"ieyou.train.ark", {"JNISurfaceTextu", "1.ui"}},
       {"v.douyu.android", {"Chrome_IOThread"}},
       {"taobao.idlefish", {"JNISurfaceTextu", "1.ui"}},
       {"encent.mm:tools", {"VizCompositorTh", "Chrome_ChildIOT", "Chrome_InProcGp", "Chrome_InProcRe", "Compositor", "Chrome_IOThread", "JavaBridge"}},
       {"com.tencent.mm",  {"VizCompositorTh", "Chrome_ChildIOT", "Chrome_InProcGp", "Chrome_InProcRe", "Compositor"}},
       {"ocessService0:0", {"CrRendererMain", "Compositor", "Chrome_ChildIOT"}},
       {"com.oplus.vip", {"VizWebView", "Chrome_ChildIOT"}},
       /* use "^X" to represent '0' '1' '2' ... */
       {"nt.mm:appbrand^X", {"VizCompositorTh", "Chrome_IOThread", "Chrome_InProcGp", "Chrome_InProcRe", "Compositor"}},
};

bool is_top(struct task_struct *p)
{
	struct cgroup_subsys_state *css;
	if (p == NULL)
		return false;

	rcu_read_lock();
	css = task_css(p, cpu_cgrp_id);
	if (!css) {
		rcu_read_unlock();
		return false;
	}
	rcu_read_unlock();

	return css->id == TOPAPP;
}

void task_rename_hook(struct task_struct *p)
{
	struct task_struct *leader = p->group_leader;
	char *p_comm = p->comm;
	const char *mark = "^X";
	int m_len = strlen(mark);
	int c_len = strlen(leader->comm);
	int i, j;

	for (i = 0; i < ARRAY_SIZE(web_target); i++) {
		struct web_target *target = &web_target[i];
		int t_len = strlen(target->val);
		char *endl = &target->val[t_len - m_len];

		if (!strncmp(leader->comm, target->val, TASK_COMM_LEN) ||
			(!strncmp(endl, mark, m_len) && c_len > 0 &&
				isdigit(leader->comm[c_len - 1]) &&
				!strncmp(leader->comm, target->val, t_len - m_len))) {
#if IS_ENABLED(CONFIG_OPLUS_FEATURE_SCHED_ASSIST)
			for (j = 0; j < CHILD_MAX; j++) {
				if (!target->desc[j])
					break;

				if (p->prio <= DEFAULT_PRIO && !strncmp(p_comm, target->desc[j], TASK_COMM_LEN)) {
					oplus_set_im_flag(p, IM_WEBVIEW);
					if(webview_debug) {
						pr_info("record webview: pid=%d comm=%s prio=%d leader_pid=%d leader_comm=%s\n",
						p->pid, p->comm, p->prio, leader->pid, leader->comm);
					}
					break;
				} else if(oplus_get_im_flag(p) == IM_WEBVIEW) {
					oplus_unset_im_flag(p, IM_WEBVIEW);
				}
			}
#endif
			break;
		}
	}
}

bool is_webview(struct task_struct *p)
{
	if (!is_top(p))
		return false;

	if (oplus_get_im_flag(p) == IM_WEBVIEW) {
		return true;
	}

	return false;
}

bool is_webview_boost(struct task_struct *p)
{
	if (is_webview(p) && task_util(p) >= MIN_BOOST_VLAUE)
		return true;

	return false;
}

extern capacity_spare_without(int cpu, struct task_struct *p);
int find_webview_cpu(struct task_struct *p)
{
	int i;
	int max_spare_cap_cpu = -1;
	int active_cpu = -1;
	unsigned long spare_cap = 0, max_spare_cap = 0;
	int index = (num_sched_clusters > 1) ? 1 : 0;
	struct cpumask *preferred_cpus = NULL;
	struct task_struct *curr = NULL;
	struct rq * rq = NULL;
	cpumask_t search_cpus = CPU_MASK_NONE;
	struct walt_sched_cluster *preferred_cluster = sched_cluster[index];

	if (!preferred_cluster)
		return -1;

	cpumask_and(&search_cpus, &p->cpus_mask, cpu_active_mask);
	preferred_cpus = &preferred_cluster->cpus;

	for_each_cpu_and(i, &search_cpus, preferred_cpus) {
		if (active_cpu == -1)
			active_cpu = i;

		if (is_reserved(i))
			continue;

		rq = cpu_rq(i);
		curr = rq->curr;
		if ((curr->prio < MAX_RT_PRIO) || is_webview(curr))
			continue;

		if (available_idle_cpu(i) || (i == task_cpu(p) && p->state == TASK_RUNNING)) {
			return i;
		}

		spare_cap = capacity_spare_without(i, p);
		if (spare_cap > max_spare_cap) {
			max_spare_cap = spare_cap;
			max_spare_cap_cpu = i;
		}
	}
	if (max_spare_cap_cpu == -1)
		max_spare_cap_cpu = active_cpu;

	return max_spare_cap_cpu;
}
