#ifndef _NONLINEAR_OPP_CAP_H_
#define _NONLINEAR_OPP_CAP_H_

struct cpufreq_policy;
struct proc_dir_entry;
extern unsigned int init_table;

int init_opp_cap_info(struct proc_dir_entry *dir);
void clear_opp_cap_info(void);
void nlopp_arch_set_freq_scale(void *data, const struct cpumask *cpus,
		unsigned long freq, unsigned long max, unsigned long *scale);
unsigned long pd_get_opp_capacity(int cpu, int opp);

#ifdef CONFIG_OPLUS_UAG_USE_TL
unsigned int pd_get_nr_caps(int cluster_id);
unsigned int pd_get_cpu_freq(int cpu, int idx);
#ifdef CONFIG_ARCH_QCOM
void uag_update_util(void *data, unsigned long util, unsigned long freq,
		unsigned long cap, unsigned long *max_util, struct cpufreq_policy *policy,
		bool *need_freq_update);
#endif
#endif
void nlopp_map_util_freq(void *data, unsigned long util, unsigned long freq,
						  struct cpumask *cpumask, unsigned long *next_freq);
#ifdef CONFIG_ARCH_MEDIATEK
void nonlinear_map_util_freq(void *data, unsigned long util, unsigned long freq,
		unsigned long cap, unsigned long *next_freq, struct cpufreq_policy *policy,
		bool *need_freq_update);
#endif /* CONFIG_ARCH_MEDIATEK */
#endif
