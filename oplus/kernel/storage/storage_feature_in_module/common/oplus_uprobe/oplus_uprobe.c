#include <linux/module.h>
#include <linux/ptrace.h>
#include <linux/uprobes.h>
#include <linux/namei.h>
#include <linux/moduleparam.h>
#include <linux/kthread.h>
#include <linux/rtc.h>
#include <linux/proc_fs.h>
#include <linux/mutex.h>
#include <uapi/linux/sched/types.h>
#include <linux/suspend.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/debugfs.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/string.h>
#include <linux/rwsem.h>
#include <linux/timer.h>
#include <linux/ctype.h>
#include <linux/kstrtox.h>
#include <linux/slab.h>
#include <linux/reboot.h>
#include <linux/signal.h>
#include <linux/kprobes.h>
#include "kernel/trace/trace_probe.h"
#define OPLUS_UPROBE_LOG_TAG "[oplus_uprobe]"

#define WRITE_BUFSIZE  4096
#define MAX_COMM_LENGTH 256
#define WHITELIST_SIZE 48
#define MAX_UPROBE_COUNT 47
#define REG_VAL_MAX 64
#define SIGVOLD_GET_LOG (SIGRTMIN + 0x13)
#define SIGOLC_GET_LOG (SIGRTMIN + 0x14)
#define SIGCONTROL_RUN_TIME (SIGRTMIN + 0x15)
#define oplus_procedure_link_pointer(regs)	((regs)->regs[30])

extern int pr_storage(const char *fmt, ...);
static LIST_HEAD(uprobe_event_list);
static struct proc_dir_entry *reliable_procfs;
static struct proc_dir_entry *storage_reliable_procfs;
struct proc_dir_entry *proc_fs_uprobe;
struct proc_dir_entry *proc_fs_uprobe_enable;
static int oplus_uprobe_enable = 0;
static atomic_t uprobe_count = ATOMIC_INIT(0);
typedef int (*uprobe_register_t)(struct inode *, loff_t, struct uprobe_consumer *);
typedef void (*uprobe_unregister_t)(struct inode *, loff_t , struct uprobe_consumer *);
typedef int (*kern_path_t)(const char *name, unsigned int flags, struct path *path);
typedef void (*path_put_t)(const struct path *path);
uprobe_register_t uprobe_register_funcptr = NULL;
uprobe_unregister_t uprobe_unregister_funcptr = NULL;
kern_path_t kern_path_funcptr = NULL;
path_put_t path_put_funcptr = NULL;
DECLARE_RWSEM(oplus_event_sem);

struct ou_param {
	struct timer_list       timer;
	unsigned int            timeout;
	unsigned int            nr_args;
	struct probe_arg        args[];
};

enum OPLUS_ACTION {
	A_PANIC,
	A_LOG,
	A_OLC,
	A_RUNTIME,
	A_RESET_MEDIA,
	A_RESET_SYSTEM,
	A_REBOOT,
	A_KILL,
	A_NONE,
};

struct oplus_uprobe {
	struct list_head        head;
	struct uprobe_consumer  consumer;
	struct path             path;
	struct inode            *inode;
	char                    *filename;
	unsigned int            userid;
	unsigned int            runtime;
	unsigned long           offset;
	int                     action;
	char*                   raw_cmd;
	struct ou_param         param;
};

struct oplus_pt_regs_offset {
	const char *name;
	int offset;
};

#define REG_OFFSET_NAME(r) {.name = #r, .offset = offsetof(struct pt_regs, r)}
#define REG_OFFSET_END {.name = NULL, .offset = 0}
#define GPR_OFFSET_NAME(r) \
	{.name = "x" #r, .offset = offsetof(struct pt_regs, regs[r])}

static const struct oplus_pt_regs_offset oplus_regoffset_table[] = {
	GPR_OFFSET_NAME(0),
	GPR_OFFSET_NAME(1),
	GPR_OFFSET_NAME(2),
	GPR_OFFSET_NAME(3),
	GPR_OFFSET_NAME(4),
	GPR_OFFSET_NAME(5),
	GPR_OFFSET_NAME(6),
	GPR_OFFSET_NAME(7),
	GPR_OFFSET_NAME(8),
	GPR_OFFSET_NAME(9),
	GPR_OFFSET_NAME(10),
	GPR_OFFSET_NAME(11),
	GPR_OFFSET_NAME(12),
	GPR_OFFSET_NAME(13),
	GPR_OFFSET_NAME(14),
	GPR_OFFSET_NAME(15),
	GPR_OFFSET_NAME(16),
	GPR_OFFSET_NAME(17),
	GPR_OFFSET_NAME(18),
	GPR_OFFSET_NAME(19),
	GPR_OFFSET_NAME(20),
	GPR_OFFSET_NAME(21),
	GPR_OFFSET_NAME(22),
	GPR_OFFSET_NAME(23),
	GPR_OFFSET_NAME(24),
	GPR_OFFSET_NAME(25),
	GPR_OFFSET_NAME(26),
	GPR_OFFSET_NAME(27),
	GPR_OFFSET_NAME(28),
	GPR_OFFSET_NAME(29),
	GPR_OFFSET_NAME(30),
	{.name = "lr", .offset = offsetof(struct pt_regs, regs[30])},
	REG_OFFSET_NAME(sp),
	REG_OFFSET_NAME(pc),
	REG_OFFSET_NAME(pstate),
	REG_OFFSET_END,
};

static char ou_whitelist[WHITELIST_SIZE][MAX_COMM_LENGTH] = {
	"/system/bin/vold",
	"/system/bin/vold_prepare_subdirs",
	"/system/bin/ueventd",
	"/system/bin/init",
	"/system/bin/rm",
	"/system/bin/cp",
	"/system/bin/mkdir",
	"/system/bin/rmdir",
	"/system/bin/chown",
	"/system/bin/chgrp",
};


static int is_event_in_whitelist(const char *event)
{
	int i;

	for (i = 0; i < WHITELIST_SIZE; i++) {
		if (strcmp(event, ou_whitelist[i]) == 0) {
			pr_err(OPLUS_UPROBE_LOG_TAG "is_event_in_whitelist true\n");
			return 0;
		}
	}

	return -1;
}

struct task_struct *get_task_struct_by_comm(const char *comm) {
	struct task_struct *task;

	for_each_process(task) {
		if (strcmp(task->comm, comm) == 0) {
			return task;
		}
	}

	return NULL;
}

struct task_struct *get_media_task_struct(const char *comm, unsigned int userid) {
	struct task_struct *task;

	for_each_process(task) {
		if (!strcmp(task->comm, comm) && (userid == (task->real_cred->uid.val/100000))) {
			return task;
		}
	}

	return NULL;
}

static int process_action_insn(int action, unsigned int	userid, unsigned int runtime)
{
	struct task_struct *task;
	kernel_siginfo_t si;

	switch (action) {
	case A_PANIC:
		panic("oplus uprobe trriger panic!");
		break;
	case A_LOG:
		task = get_task_struct_by_comm("Binder:vold");
		if (!task) {
			pr_err(OPLUS_UPROBE_LOG_TAG "No task_struct found for process\n");
			pr_storage(OPLUS_UPROBE_LOG_TAG "No task_struct found for process\n");
			break;
		}
		send_sig_info(SIGVOLD_GET_LOG, SEND_SIG_PRIV, task);
		break;
	case A_OLC:
		task = get_task_struct_by_comm("Binder:vold");
		if (!task) {
			pr_err(OPLUS_UPROBE_LOG_TAG "No task_struct found for process\n");
			pr_storage(OPLUS_UPROBE_LOG_TAG "No task_struct found for process\n");
			break;
		}
		send_sig_info(SIGOLC_GET_LOG, SEND_SIG_PRIV, task);
		break;
	case A_RUNTIME:
		clear_siginfo(&si);
		si.si_signo = SIGCONTROL_RUN_TIME;
		si.si_errno = 0;
		si.si_code  = -1;
		si.si_int = runtime;
		task = get_task_struct(current);
		if (!task) {
			pr_err(OPLUS_UPROBE_LOG_TAG "No task_struct found for process\n");
			pr_storage(OPLUS_UPROBE_LOG_TAG "No task_struct found for process\n");
			break;
		}
		send_sig_info(SIGCONTROL_RUN_TIME, &si, task);
		put_task_struct(current);
		break;
	case A_RESET_MEDIA:
		task = get_media_task_struct("rs.media.module", userid);
		if (!task) {
			pr_err(OPLUS_UPROBE_LOG_TAG "No task_struct found for process\n");
			pr_storage(OPLUS_UPROBE_LOG_TAG "No task_struct found for process\n");
			break;
		}
		send_sig_info(SIGKILL, SEND_SIG_PRIV, task);
		break;
	case A_RESET_SYSTEM:
		task = get_task_struct_by_comm("system_server");
		if (!task) {
			pr_err(OPLUS_UPROBE_LOG_TAG "No task_struct found for process\n");
			pr_storage(OPLUS_UPROBE_LOG_TAG "No task_struct found for process\n");
			break;
		}
		send_sig_info(SIGKILL, SEND_SIG_PRIV, task);
		break;
	case A_REBOOT:
		kernel_restart(NULL);
		break;
	case A_NONE:
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static void uprobe_timed_out_timer(struct timer_list *t)
{
	struct ou_param *op = container_of(t, struct ou_param, timer);
	struct oplus_uprobe *ou = container_of(op, struct oplus_uprobe, param);

	pr_storage(OPLUS_UPROBE_LOG_TAG "uprobe_timed_out_timer\n");
	if (!ou) {
		pr_err(OPLUS_UPROBE_LOG_TAG "get uprobe object failed\n");
		return;
	}
	if (ou->action != A_REBOOT) {
		process_action_insn(ou->action, ou->userid, ou->runtime);
	}
}

static int timer_handler_pre(struct uprobe_consumer *self, struct pt_regs *regs)
{
	struct oplus_uprobe *ou = container_of(self, struct oplus_uprobe, consumer);

	timer_setup(&ou->param.timer, uprobe_timed_out_timer, 0);
	pr_storage(OPLUS_UPROBE_LOG_TAG "timer_handler_pre\n");
	mod_timer(&ou->param.timer, jiffies + msecs_to_jiffies(ou->param.timeout));
    return 0;
}

static int timer_handler_ret(struct uprobe_consumer *self,
                                unsigned long func,
                                struct pt_regs *regs)
{
	struct oplus_uprobe *ou = container_of(self, struct oplus_uprobe, consumer);

	pr_storage(OPLUS_UPROBE_LOG_TAG "oplus_uprobe handler_ret\n");
	del_timer_sync(&ou->param.timer);
	return 0;
}

static inline void oplus_regs_set_return_value(struct pt_regs *regs, unsigned long rc)
{
  	regs->regs[0] = rc;
}

static inline void oplus_instruction_pointer_set(struct pt_regs *regs,
  		unsigned long val)
{
  	regs->pc = val;
}

static int oplus_regs_query_register_offset(const char *name)
{
	const struct oplus_pt_regs_offset *roff;

	for (roff = oplus_regoffset_table; roff->name != NULL; roff++)
		if (!strcmp(roff->name, name))
			return roff->offset;
	return -EINVAL;
}

static int
process_fetch_insn(struct fetch_insn *code, void *rec, void *dest,
		   void *base,int action, unsigned int userid, unsigned int runtime)
{
	struct pt_regs *regs = rec;
	unsigned long long val;
	unsigned long long comp_val;
	unsigned long long set_val;
	unsigned int offset;
	char *reg;
	char *reg_val;
	int ret;
	struct task_struct *task;
	pid_t current_tgid, tgid;
	char *reg_string;

	switch (code->op) {
	case FETCH_OP_REG:
		reg_string = kmalloc(REG_VAL_MAX, GFP_KERNEL);
		strncpy(reg_string, (char*)code->data, REG_VAL_MAX);
		reg = strsep(&reg_string, "=");
		if (!reg) {
			kfree(reg_string);
			pr_err(OPLUS_UPROBE_LOG_TAG "parse register reg failed\n");
			return -EINVAL;
		}
		reg_val = strsep(&reg_string, "=");
		if (!reg_val) {
			kfree(reg_string);
			pr_err(OPLUS_UPROBE_LOG_TAG "parse register reg_val failed\n");
			return -EINVAL;
		}
		pr_storage(OPLUS_UPROBE_LOG_TAG"oplus_parse_arg reg(%s) regval(%s)\n", reg,  reg_val);
		if (isdigit(reg_val[0])) {
			ret = kstrtoull(reg_val, 0, &comp_val);
			if (ret) {
				kfree(reg_string);
				pr_err(OPLUS_UPROBE_LOG_TAG "parse register kstrtoull failed\n");
				return -EINVAL;
			}
		} else {
			kfree(reg_string);
			return -EINVAL;
		}

		offset = oplus_regs_query_register_offset(reg);
		if (offset < 0) {
			kfree(reg_string);
			pr_err(OPLUS_UPROBE_LOG_TAG "parse register failed\n");
			return -EINVAL;
		}

		val = regs_get_register(regs, offset);
		pr_storage(OPLUS_UPROBE_LOG_TAG "FETCH_OP_REG val(0x%llx) offset(%d) comp_val(0x%llx)\n", val, offset, comp_val);
		if (comp_val == val) {
			pr_storage(OPLUS_UPROBE_LOG_TAG "FETCH_OP_REG val(0x%llx) comp_val(0x%llx)\n", val, comp_val);
			process_action_insn(action, userid, runtime);
		}
		kfree(reg_string);
		break;
	case FETCH_OP_MOD_BF:
		reg_string = kmalloc(REG_VAL_MAX, GFP_KERNEL);
		strncpy(reg_string, (char*)code->data, REG_VAL_MAX);
		reg = strsep(&reg_string, "=");
		if (!reg) {
			kfree(reg_string);
			pr_err(OPLUS_UPROBE_LOG_TAG "parse register reg failed\n");
			return -EINVAL;
		}
		reg_val = strsep(&reg_string, "=");
		if (!reg_val) {
			kfree(reg_string);
			pr_err(OPLUS_UPROBE_LOG_TAG "parse register reg_val failed\n");
			return -EINVAL;
		}
		if (isdigit(reg_val[0])) {
			ret = kstrtoull(reg_val, 0, &set_val);
			if (ret) {
				kfree(reg_string);
				pr_err(OPLUS_UPROBE_LOG_TAG "parse register kstrtoull failed\n");
				return -EINVAL;
			}
		} else {
			kfree(reg_string);
			return -EINVAL;
		}

		offset = oplus_regs_query_register_offset(reg);
		pr_storage(OPLUS_UPROBE_LOG_TAG"oplus_parse_arg 2 reg(%s) offset(%d) regval(0x%llx)\n", reg, offset, set_val);
		if (offset < 0) {
			pr_err(OPLUS_UPROBE_LOG_TAG "parse register failed\n");
			return -EINVAL;
		}

		offset >>= 3;
		pr_storage(OPLUS_UPROBE_LOG_TAG "FETCH_OP_MOD_BF offset(%d) set_val(0x%llx)\n", offset, set_val);
		pt_regs_write_reg(regs, offset, set_val);
		kfree(reg_string);
		break;
	case FETCH_OP_DATA:
		oplus_regs_set_return_value(regs, code->offset);
		oplus_instruction_pointer_set(regs, oplus_procedure_link_pointer(regs));
		val = regs_return_value(regs);
		pr_storage(OPLUS_UPROBE_LOG_TAG "FETCH_OP_DATA set ret val(%d) offset(%d)\n", val, code->offset);
		process_action_insn(action, userid, runtime);
		break;
	case FETCH_OP_RETVAL:
		val = regs_return_value(regs);
		if(val == code->offset) {
			pr_storage(OPLUS_UPROBE_LOG_TAG "FETCH_OP_RETVAL get ret val(%d) offset(%d)\n", val, code->offset);
			process_action_insn(action, userid, runtime);
		}
		break;
	case FETCH_OP_COMM:
		current_tgid = task_tgid_nr(current);
		pr_storage(OPLUS_UPROBE_LOG_TAG "FETCH_OP_COMM current_tgid(%d) \n",current_tgid);
		task = get_task_struct_by_comm(code->data);
		if (!task) {
			pr_err(OPLUS_UPROBE_LOG_TAG "No task_struct found for process\n");
			break;
		}
		tgid = task->tgid;
		pr_storage(OPLUS_UPROBE_LOG_TAG "FETCH_OP_COMM tgid(%d) \n",tgid);
		if (current_tgid == tgid) {
			pr_err(OPLUS_UPROBE_LOG_TAG "FETCH_OP_COMM\n");
			pr_storage(OPLUS_UPROBE_LOG_TAG "FETCH_OP_COMM\n");
			process_action_insn(action, userid, runtime);
		}
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int none_handler_pre(struct uprobe_consumer *self, struct pt_regs *regs)
{
	struct oplus_uprobe *ou = container_of(self, struct oplus_uprobe, consumer);

	process_action_insn(ou->action, ou->userid, ou->runtime);
	return 0;

}

static int keyvalue_handler_pre(struct uprobe_consumer *self, struct pt_regs *regs)
{
	struct oplus_uprobe *ou = container_of(self, struct oplus_uprobe, consumer);
	int argc = ou->param.nr_args;
	int action = ou->action;
	unsigned int userid = ou->userid;
	unsigned int runtime = ou->runtime;
	int i;
	for (i = 0; i < argc && i < MAX_TRACE_ARGS; i++) {
		struct probe_arg *parg = &ou->param.args[i];
		if (parg->code->op != FETCH_OP_RETVAL)
			process_fetch_insn(parg->code, regs, NULL, NULL, action, userid, runtime);
	}
	pr_storage(OPLUS_UPROBE_LOG_TAG "keyvalue_handler_pre end\n");

	return 0;

}

static int keyvalue_handler_ret(struct uprobe_consumer *self, unsigned long func, struct pt_regs *regs)
{
	struct oplus_uprobe *ou = container_of(self, struct oplus_uprobe, consumer);
	int argc = ou->param.nr_args;
	int action = ou->action;
	unsigned int userid = ou->userid;
	unsigned int runtime = ou->runtime;
	int i;
	for (i = 0; i < argc && i < MAX_TRACE_ARGS; i++) {
		struct probe_arg *parg = &ou->param.args[i];
		if (parg->code->op == FETCH_OP_RETVAL)
			process_fetch_insn(parg->code, regs, NULL, NULL, action, userid, runtime);
	}
	pr_storage(OPLUS_UPROBE_LOG_TAG "keyvalue_handler_ret end\n");

	return 0;
}

static int parse_probe_vars(char *arg, const struct fetch_type *t,
			struct fetch_insn *code, int offs, char *value, unsigned int flags)
{
	int retval;
	unsigned long param;
	int ret = 0;
	int len;

	if (!strcmp(arg, "retval") && (flags & TPARG_FL_RETURN)) {
		code->op = FETCH_OP_RETVAL;
		ret = kstrtoint(value, 0, &retval);
		if (ret) {
			pr_err(OPLUS_UPROBE_LOG_TAG "parse get retvalue failed\n");
			goto inval_var;
		}
		code->offset = retval;
	} else if (!strcmp(arg, "setret")) {
		pr_storage(OPLUS_UPROBE_LOG_TAG "setret\n");
		if(oplus_uprobe_enable == 1) {
			code->op = FETCH_OP_DATA;
			ret = kstrtoint(value, 0, &retval);
			if (ret) {
				pr_err(OPLUS_UPROBE_LOG_TAG "parse set retvalue failed\n");
				goto inval_var;
			}
			code->offset = retval;
			return ret;
		}
		return -EINVAL;
	} else if ((len = str_has_prefix(arg, "stack"))) {
		if (arg[len] == '\0') {
			code->op = FETCH_OP_STACKP;
		} else if (isdigit(arg[len])) {
			ret = kstrtoul(arg + len, 10, &param);
			if (ret) {
				goto inval_var;
			} else {
				code->op = FETCH_OP_STACK;
				code->param = (unsigned int)param;
			}
		} else
			goto inval_var;
	} else if (strcmp(arg, "comm") == 0) {
		code->op = FETCH_OP_COMM;
		code->data = kstrdup(value, GFP_KERNEL);
		if (!code->data)
			return -ENOMEM;
	} else
		goto inval_var;

	return ret;

inval_var:
	return -EINVAL;
}

static int parse_probe_reg(char *arg, struct fetch_insn *code)
{
	char *value;
	int ret = 0;

	if (strstr(arg, "setreg")) {
		pr_storage(OPLUS_UPROBE_LOG_TAG "parse register setreg\n");
		if(oplus_uprobe_enable == 1) {
			value = strrchr(arg, ':');
			if (!value) {
				pr_err(OPLUS_UPROBE_LOG_TAG "parse register value failed\n");
				return -EINVAL;
			}
			*value++ = '\0';
			code->op = FETCH_OP_MOD_BF;
			code->data = kstrdup(value, GFP_KERNEL);
			if (!code->data)
				return -ENOMEM;
			return ret;
		}
		return -EINVAL;
	} else if (strstr(arg, "getreg")) {
		pr_storage(OPLUS_UPROBE_LOG_TAG "parse register getreg\n");
		value = strrchr(arg, ':');
		if (!value) {
			pr_err(OPLUS_UPROBE_LOG_TAG "parse register reg failed\n");
			return -EINVAL;
		}
		*value++ = '\0';
		code->op = FETCH_OP_REG;
		code->data = kstrdup(value, GFP_KERNEL);
		if (!code->data )
			return -ENOMEM;
	} else {
			pr_err(OPLUS_UPROBE_LOG_TAG "parse register ending failed arg(%s)\n",arg);
			return -EINVAL;
	}

	return ret;
}
static int oplus_parse_arg(char *arg, const struct fetch_type *type,
								struct fetch_insn **pcode, struct fetch_insn *end,
								unsigned int flags, int offs, bool is_return)
{
	struct fetch_insn *code = *pcode;
	unsigned long param;
	long offset = 0;
	int ret = 0;
	char *value;

	switch (arg[0]) {
	case '#':
		value = strrchr(arg + 1, '=');
		if (!value) {
			return -EINVAL;
		}
		*value++ = '\0';
		ret = parse_probe_vars(arg + 1, type, code, offs, value, is_return ? TPARG_FL_RETURN : 0);
		break;

	case '%':
		ret = parse_probe_reg(arg + 1, code);
		break;

	case '@':
		pr_storage(OPLUS_UPROBE_LOG_TAG"oplus_parse_arg 3 arg(%s)\n", arg + 1);
		if (isdigit(arg[1])) {
			ret = kstrtoul(arg + 1, 0, &param);
			if (ret) {
				pr_err(OPLUS_UPROBE_LOG_TAG "parse memory-offset failed\n");
				break;
			}
			code->op = FETCH_OP_IMM;
			code->immediate = param;
		} else if (arg[1] == '+') {
			if (flags & TPARG_FL_KERNEL) {
				pr_err(OPLUS_UPROBE_LOG_TAG "parse memory-offset failed\n");
				return -EINVAL;
			}
			ret = kstrtol(arg + 2, 0, &offset);
			if (ret) {
				pr_err(OPLUS_UPROBE_LOG_TAG "parse memory-offset failed\n");
				break;
			}

			code->op = FETCH_OP_FOFFS;
			code->immediate = (unsigned long)offset;  // imm64?
		} else {
			code->op = FETCH_NOP_SYMBOL;
			code->data = kstrdup(arg + 1, GFP_KERNEL);
			if (!code->data)
				return -ENOMEM;
			if (++code == end) {
				pr_err(OPLUS_UPROBE_LOG_TAG "too many options\n");
				return -EINVAL;
			}
			code->op = FETCH_OP_IMM;
			code->immediate = 0;
		}
		if (++code == end) {
			pr_err(OPLUS_UPROBE_LOG_TAG "too many options\n");
			return -EINVAL;
		}

		*pcode = code;
		code->op = FETCH_OP_DEREF;
		code->offset = offset;
		break;
	default:
		return -EINVAL;

	}
	if (ret) {
		pr_err(OPLUS_UPROBE_LOG_TAG "BAD_FETCH_ARG\n");
		ret = -EINVAL;
	}
	return ret;
}

int parse_keyvalue_body_arg(struct ou_param *op, int i, char *argv, bool is_return)
{
	struct probe_arg *parg = &op->args[i];
	int ret = 0;
	char *arg;
	unsigned int flags = TPARG_FL_KERNEL;
	arg = strchr(argv, ':');
	if (!arg) {
		pr_err(OPLUS_UPROBE_LOG_TAG "key_value param is invalid, find no:\n");
		goto err;
	}
	*arg++ = '\0';
	pr_storage(OPLUS_UPROBE_LOG_TAG "parse arg(%s)\n", arg);
	parg->code = kzalloc(sizeof(*parg->code), GFP_KERNEL);
	if (!parg->code) {
		pr_err(OPLUS_UPROBE_LOG_TAG "kcalloc failed\n");
		goto err;
	}

	ret = oplus_parse_arg(arg, parg->type, &parg->code, &parg->code[FETCH_INSN_MAX - 1], flags, 0, is_return);
	if (ret) {
		pr_err(OPLUS_UPROBE_LOG_TAG "parse arg failed\n");
		goto err;
	}

	return 0;

err:
	if(parg->code)
		kfree(parg->code);
	return -EINVAL;
}

static int kern_path_fn(const char *name, unsigned int flags, struct path *path)
{
	int ret = 0;

	ret = kern_path_funcptr(name, flags, path);

	return ret;
}

static void path_put_fn(const struct path *path)
{
	path_put_funcptr(path);
}

static struct oplus_uprobe* parse_uprobe_cmd(int argc, char **argv)
{
	char *arg;
	struct path path;
	char *event_name = "uprobe";
	char *filename;
	char *action;
	unsigned long offset;
	struct oplus_uprobe *ou = NULL;
	int ret;
	int i;
	unsigned long timeout;
	unsigned long userid;
	unsigned long runtime;
	bool is_return = false;
	int type;

	if (argc < 4) {
		pr_err(OPLUS_UPROBE_LOG_TAG"argc %d is not invalid\n", argc);
        return NULL;
	}

	type = argv[0][0];
	switch (type) {
	case 'r':
		is_return = true;
		break;
	case 'p':
		break;
	default:
		pr_err(OPLUS_UPROBE_LOG_TAG "cmd type(%c) is not supported\n", type);
		return NULL;
	}
	if (argv[0][1] == ':') {
		event_name = &argv[0][2];
	}

	filename = kstrdup(argv[1], GFP_KERNEL);
	if (!filename)
		return NULL;

	arg = strrchr(filename, ':');
	if (!arg || !isdigit(arg[1])) {
		pr_err(OPLUS_UPROBE_LOG_TAG "filename %s is not invalid\n", filename);
		goto err;
	}
	*arg++ = '\0';

	ret = kern_path_fn(filename, LOOKUP_FOLLOW, &path);
	if (ret < 0) {
		pr_err(OPLUS_UPROBE_LOG_TAG "kern_path failed, ret %d, filename %s\n", ret, filename);
		goto err;
	}
	if (is_event_in_whitelist(filename)) {
		pr_err(OPLUS_UPROBE_LOG_TAG "is_event_in_whitelist false\n");
		goto err;
	}
	ret = kstrtoul(arg, 0, &offset);
	if (ret) {
		pr_err(OPLUS_UPROBE_LOG_TAG "offset param is invalid, %s\n", arg);
		goto err;
	}
	argc -= 3;
	pr_storage(OPLUS_UPROBE_LOG_TAG "filename(%s), offset(0x%x) argc(%d)\n", filename, offset, argc);
	ou = kzalloc(struct_size(ou, param.args, argc), GFP_KERNEL);
	if (!ou) {
		pr_err(OPLUS_UPROBE_LOG_TAG "alloc ou failed\n");
		goto err;
	}
	argv += 2;
	pr_storage(OPLUS_UPROBE_LOG_TAG "argv[0](%s)\n", argv[0]);
	if (0 == strncmp(argv[0], "timer", strlen("timer"))) {
		arg = strrchr(argv[0], ':');
		if (!arg || !isdigit(arg[1])) {
			pr_err(OPLUS_UPROBE_LOG_TAG "argv[0] param is invalid, %s\n", argv[0]);
			goto err;
		}

		*arg++ = '\0';
		ret = kstrtoul(arg, 0, &timeout);
		if (ret) {
			pr_err(OPLUS_UPROBE_LOG_TAG "timer timeout param is invalid (%s)\n", arg);
			goto err;
		}
		ou->param.timeout = timeout;
		ou->consumer.handler = timer_handler_pre;
		ou->consumer.ret_handler = timer_handler_ret;
	} else if (0 == strncmp(argv[0], "key_value", strlen("key_value"))) {
		//argv += 2;
		pr_storage(OPLUS_UPROBE_LOG_TAG "key_value argc(%d)\n", argc);
		for (i = 0; i < argc && i < MAX_TRACE_ARGS; i++) {
			ret = parse_keyvalue_body_arg(&ou->param, i, argv[i], is_return);
			if (ret)
				goto err;
		}
		ou->param.nr_args = argc;
		ou->consumer.handler = keyvalue_handler_pre;
		if (is_return)
			ou->consumer.ret_handler = keyvalue_handler_ret;
	} else if (0 == strncmp(argv[0], "none", strlen("none"))) {
		ou->consumer.handler = none_handler_pre;
	} else {
		pr_err(OPLUS_UPROBE_LOG_TAG " invalid argv[2] (%s)\n", argv[2]);
		goto err;
	}

	action = argv[argc];
	if (!action) {
		pr_err(OPLUS_UPROBE_LOG_TAG "action is NULL \n");
		goto err;
	}
	pr_storage(OPLUS_UPROBE_LOG_TAG "action(%s)\n", action);
	if (0 == strncmp(action, "panic", strlen("panic"))) {
		ou->action = A_PANIC;
	} else if(0 == strncmp(action, "vlog", strlen("vlog"))) {
		ou->action = A_LOG;
	} else if(0 == strncmp(action, "rtime", strlen("rtime"))) {
		arg = strrchr(action, ':');
		if (!arg || !isdigit(arg[1])) {
			pr_err(OPLUS_UPROBE_LOG_TAG "action is invalid, %s\n", action);
			goto err;
		}

		*arg++ = '\0';
		ret = kstrtoul(arg, 0, &runtime);
		if (ret) {
			pr_err(OPLUS_UPROBE_LOG_TAG "timer userid is invalid (%s)\n", arg);
			goto err;
		}
		ou->runtime = runtime;
		ou->action = A_RUNTIME;
	} else if(0 == strncmp(action, "olog", strlen("olog"))) {
		ou->action = A_OLC;
	} else if(0 == strncmp(action, "rmedia", strlen("rmedia"))) {
		arg = strrchr(action, ':');
		if (!arg || !isdigit(arg[1])) {
			pr_err(OPLUS_UPROBE_LOG_TAG "action is invalid, %s\n", action);
			goto err;
		}

		*arg++ = '\0';
		ret = kstrtoul(arg, 0, &userid);
		if (ret) {
			pr_err(OPLUS_UPROBE_LOG_TAG "timer userid is invalid (%s)\n", arg);
			goto err;
		}
		ou->userid = userid;
		ou->action = A_RESET_MEDIA;
	} else if(0 == strncmp(action, "rsystem", strlen("rsystem"))) {
		ou->action = A_RESET_SYSTEM;
	} else if(0 == strncmp(action, "reboot", strlen("reboot"))) {
		ou->action = A_REBOOT;
	} else if(0 == strncmp(action, "none", strlen("none"))) {
		ou->action = A_NONE;
	} else {
		pr_storage(OPLUS_UPROBE_LOG_TAG "invalid action\n");
		pr_err(OPLUS_UPROBE_LOG_TAG "invalid action\n");
		goto err;
	}
	ou->offset = offset;
	ou->path = path;
	ou->filename = filename;
	return ou;

err:
	if (ou)
		kfree(ou);

	if (filename)
		kfree(filename);

	path_put_fn(&path);
	return NULL;
}

void ou_param_cleanup(struct probe_arg *arg)
{
	struct fetch_insn *code = arg->code;

	if (code->op == FETCH_OP_COMM) {
		pr_err(OPLUS_UPROBE_LOG_TAG "ou_param_cleanup code->data \n");
		kfree(code->data);
	}

	kfree(arg->code);
}

int __nocfi uprobe_register_fn(struct inode *inode, unsigned long offset, struct uprobe_consumer *consumer)
{
	int ret = 0;

	ret = uprobe_register_funcptr(inode, offset, consumer);

	return ret;
}

void __nocfi uprobe_unregister_fn(struct inode *inode, unsigned long offset, struct uprobe_consumer *consumer)
{
	uprobe_unregister_funcptr(inode, offset, consumer);
}

static void delete_uprobe_cmd(int argc, char **argv)
{
	int ret;
	char *arg;
	char *filename;
	unsigned long offset;
	struct oplus_uprobe *comp;
	struct path path;
	struct inode *comp_inode;
	int i;

	if (argc < 2) {
		pr_err(OPLUS_UPROBE_LOG_TAG "argc %d is not invalid\n", argc);
		return;
	}

	filename = argv[1];
	if (!strchr(filename, '/')) {
		pr_err(OPLUS_UPROBE_LOG_TAG "filename %s is not invalid\n", filename);
		return;
	}

	arg = strrchr(filename, ':');
	if (!arg || !isdigit(arg[1])) {
		pr_err(OPLUS_UPROBE_LOG_TAG "filename %s is not invalid\n", filename);
		return;
	}
	*arg++ = '\0';

	ret = kern_path_fn(filename, LOOKUP_FOLLOW, &path);
	if (ret < 0) {
		pr_err(OPLUS_UPROBE_LOG_TAG "kern_path failed, ret %d, filename %s\n", ret, filename);
		return;
	}

	ret = kstrtoul(arg, 0, &offset);
	if (ret) {
		pr_err(OPLUS_UPROBE_LOG_TAG "offset param is invalid, %s\n", arg);
		return;
	}
	down_write(&oplus_event_sem);
	list_for_each_entry(comp, &uprobe_event_list, head) {
		comp_inode = d_real_inode(comp->path.dentry);
		if (comp_inode == d_real_inode(path.dentry) && (comp->offset == offset)) {
			list_del(&comp->head);
			uprobe_unregister_fn(comp->inode, comp->offset, &comp->consumer);
			comp->inode = NULL;
			for (i = 0; i < comp->param.nr_args && i < MAX_TRACE_ARGS; i++) {
				ou_param_cleanup(&comp->param.args[i]);
			}
			kfree(comp->filename);
			kfree(comp->raw_cmd);
			kfree(comp);
			atomic_dec(&uprobe_count);
			pr_storage(OPLUS_UPROBE_LOG_TAG "ou_param_cleanup done \n");
			break;
		}
	}
	up_write(&oplus_event_sem);
}

static int oplus_uprobe_register(struct oplus_uprobe *ou)
{
	int ret;

	ou->inode = d_real_inode(ou->path.dentry);
	pr_err(OPLUS_UPROBE_LOG_TAG "oplus_uprobe_register\n");
	ret = uprobe_register_fn(ou->inode, ou->offset, &ou->consumer);
	if (ret)
		ou->inode = NULL;

	return ret;
}

static ssize_t oplus_uprobe_proc_write(struct file *file, const char __user *buffer,
			    size_t count, loff_t *ppos)
{
	char *kbuf = NULL;
	int argc = 0;
	char **argv = NULL;
	struct oplus_uprobe *ou = NULL;
	struct oplus_uprobe *comp;
	struct inode *comp_inode;
	int ret;

    if (count > WRITE_BUFSIZE) {
        return -EINVAL;
	}
	kbuf = kmalloc(WRITE_BUFSIZE, GFP_KERNEL);
	if (!kbuf) {
		return -ENOMEM;
	}

	if (copy_from_user(kbuf, buffer, count)) {
		pr_err(OPLUS_UPROBE_LOG_TAG "copy data from user buffer failed\n");
		goto err;
	}

	kbuf[count] = '\0';

	argv = argv_split(GFP_KERNEL, kbuf, &argc);
	if (!argv) {
		pr_err(OPLUS_UPROBE_LOG_TAG "argv_split fail\n");
		goto err;
	}

	if (argv[0][0] == '-') {
		delete_uprobe_cmd(argc, argv);
		goto out;
	}

    if (atomic_read(&uprobe_count) > MAX_UPROBE_COUNT) {
		pr_err(OPLUS_UPROBE_LOG_TAG "uprobe_count Maximum limit\n");
		pr_storage(OPLUS_UPROBE_LOG_TAG "uprobe_count Maximum limit uprobe_count(%d)\n", uprobe_count);
        return -EINVAL;
	}

	ou = parse_uprobe_cmd(argc, argv);
	if (!ou) {
		pr_err(OPLUS_UPROBE_LOG_TAG "parse_uprobe_cmd fail\n");
		goto err;
	}
	ou->raw_cmd = kbuf;
	INIT_LIST_HEAD(&ou->head);

	down_write(&oplus_event_sem);
	list_for_each_entry(comp, &uprobe_event_list, head) {
		comp_inode = d_real_inode(comp->path.dentry);
		if (comp_inode == d_real_inode(ou->path.dentry) && (comp->offset == ou->offset)) {
			up_write(&oplus_event_sem);
			pr_storage(OPLUS_UPROBE_LOG_TAG "the same event exist, filename %s, offset 0x%x\n", comp->filename, comp->offset);
			goto err;
		}
	}
	list_add_tail(&ou->head, &uprobe_event_list);
	up_write(&oplus_event_sem);

	ret = oplus_uprobe_register(ou);
	if (ret) {
		kfree(ou->filename);
		//uprobe_unregister(ou->inode, ou->offset, &ou->consumer);
		uprobe_unregister_fn(ou->inode, ou->offset, &ou->consumer);
		down_write(&oplus_event_sem);
		list_del(&ou->head);
		up_write(&oplus_event_sem);
		pr_err(OPLUS_UPROBE_LOG_TAG "create_uprobe_cmd failed, ret %d\n", ret);
		goto err;
	}
	atomic_inc(&uprobe_count);
out:
	argv_free(argv);
	pr_storage(OPLUS_UPROBE_LOG_TAG "proc write succeed\n");
	return count;

err:
	if (kbuf) {
		kfree(kbuf);
	}

	if (argv) {
		argv_free(argv);
	}

	if (ou) {
		kfree(ou);
	}

	pr_storage(OPLUS_UPROBE_LOG_TAG "proc write failed\n");
	return -1;
}

static int oplus_uprobe_proc_show(struct seq_file *m, void *v)
{
	struct oplus_uprobe* tmp;

	down_write(&oplus_event_sem);
	list_for_each_entry(tmp, &uprobe_event_list, head) {
		seq_printf(m, "%s\n", tmp->raw_cmd);
		pr_storage("%s\n", tmp->raw_cmd);
	}
	up_write(&oplus_event_sem);

	return 0;
}

static int oplus_uprobe_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, oplus_uprobe_proc_show, inode->i_private);
}

static struct proc_ops oplus_uprobe_proc_ops = {
	.proc_open			= oplus_uprobe_proc_open,
	.proc_read			= seq_read,
	.proc_write			= oplus_uprobe_proc_write,
	.proc_release			= single_release,
	.proc_lseek			= default_llseek,
};

static ssize_t uprobe_enable_proc_write(struct file *file, const char __user *buf,
		size_t count, loff_t *off)
{
	char str[3] = {0};

	if (count > 2 || count < 1) {
		return -EINVAL;
	}

	if (copy_from_user(str, buf, count)) {
		pr_err(OPLUS_UPROBE_LOG_TAG "copy_from_user failed\n");
		return -EFAULT;
	}

	if (unlikely(!strncmp(str, "1", 1))) {
		pr_info(OPLUS_UPROBE_LOG_TAG "oplus uprobe enable\n");
		oplus_uprobe_enable = 1;
	} else {
		pr_info(OPLUS_UPROBE_LOG_TAG "oplus uprobe disabled\n");
		oplus_uprobe_enable = 0;
	}

	return (ssize_t)count;
}

static int uprobe_enable_show(struct seq_file *s, void *data)
{
	if (oplus_uprobe_enable == 1)
		seq_printf(s, "%d\n", 1);
	else
		seq_printf(s, "%d\n", 0);

	return 0;
}

static int uprobe_enable_proc_open(struct inode *inodp, struct file *filp)
{
	return single_open(filp, uprobe_enable_show, inodp);
}

static struct proc_ops uprobe_enable_fops = {
	.proc_open		= uprobe_enable_proc_open,
	.proc_read		= seq_read,
	.proc_write		= uprobe_enable_proc_write,
	.proc_release		= single_release,
	.proc_lseek		= default_llseek,
};

static struct kprobe uprobe_register_kp = {
	.symbol_name = "uprobe_register"
};
static struct kprobe uprobe_unregister_kp = {
	.symbol_name = "uprobe_unregister"
};

static struct kprobe kern_path_kp = {
	.symbol_name = "kern_path"
};
static struct kprobe path_put_kp = {
	.symbol_name = "path_put"
};

static int __init oplus_uprobe_init(void) {

	int ret;
	ret = register_kprobe(&uprobe_register_kp);
	if (ret < 0) {
		pr_err(OPLUS_UPROBE_LOG_TAG" register_kprobe uprobe_register_kp failed, return %d\n", ret);
		return ret;
	} else {
		uprobe_register_funcptr = (uprobe_register_t)uprobe_register_kp.addr;
		unregister_kprobe(&uprobe_register_kp);
		pr_err(OPLUS_UPROBE_LOG_TAG" uprobe_register func addr:0x%lx\n", uprobe_register_funcptr);
	}

	ret = register_kprobe(&uprobe_unregister_kp);
	if (ret < 0) {
		pr_err(OPLUS_UPROBE_LOG_TAG" register_kprobe uprobe_unregister_kp failed, return %d\n", ret);
		return ret;
	} else {
		uprobe_unregister_funcptr = (uprobe_unregister_t)uprobe_unregister_kp.addr;
		unregister_kprobe(&uprobe_unregister_kp);
		pr_err(OPLUS_UPROBE_LOG_TAG" uprobe_unregister func addr:0x%lx\n", uprobe_unregister_funcptr);
	}

	ret = register_kprobe(&kern_path_kp);
	if (ret < 0) {
		pr_err(OPLUS_UPROBE_LOG_TAG" register_kprobe kern_path_kp failed, return %d\n", ret);
		return ret;
	} else {
		kern_path_funcptr = (kern_path_t)kern_path_kp.addr;
		unregister_kprobe(&kern_path_kp);
		pr_err(OPLUS_UPROBE_LOG_TAG" uprobe_register func addr:0x%lx\n", kern_path_kp);
	}

	ret = register_kprobe(&path_put_kp);
	if (ret < 0) {
		pr_err(OPLUS_UPROBE_LOG_TAG" register_kprobe path_put_kp failed, return %d\n", ret);
		return ret;
	} else {
		path_put_funcptr = (path_put_t)uprobe_unregister_kp.addr;
		unregister_kprobe(&path_put_kp);
		pr_err(OPLUS_UPROBE_LOG_TAG" uprobe_unregister func addr:0x%lx\n", path_put_kp);
	}

	if(NULL == uprobe_register_funcptr || NULL == uprobe_unregister_funcptr) {
		pr_err(OPLUS_UPROBE_LOG_TAG" uprobe_register or uprobe_unregister is NULL\n");
		return -EFAULT;
	}

	reliable_procfs = proc_mkdir("oplus_reliable", NULL);
	if (!reliable_procfs) {
		pr_err(OPLUS_UPROBE_LOG_TAG" Failed to create oplus_reliable procfs\n");
		return -EFAULT;
	}

	storage_reliable_procfs = proc_mkdir("storage_reliable", reliable_procfs);
	if (storage_reliable_procfs == NULL) {
		pr_err(OPLUS_UPROBE_LOG_TAG" Failed to create storage_reliable procfs\n");
		return -EFAULT;
	}

	proc_fs_uprobe = proc_create("oplus_uprobe", 0644, storage_reliable_procfs, &oplus_uprobe_proc_ops);
	if (proc_fs_uprobe == NULL) {
		pr_err(OPLUS_UPROBE_LOG_TAG" Failed to create file oplus_storage/storage_reliable/oplus_uprobe\n");
		return -EFAULT;
	}

	proc_fs_uprobe_enable = proc_create("uprobe_enable", 0600, storage_reliable_procfs, &uprobe_enable_fops);
	if (proc_fs_uprobe_enable == NULL) {
		pr_err(OPLUS_UPROBE_LOG_TAG" Failed to create file oplus_storage/storage_reliable/uprobe_enable\n");
		return -EFAULT;
	}

	return 0;
}

static void __exit oplus_uprobe_exit(void)
{

	if(NULL == uprobe_register_funcptr || NULL == uprobe_unregister_funcptr) {
		pr_err(OPLUS_UPROBE_LOG_TAG" uprobe_register or uprobe_unregister is NULL\n");
		return;
	}


	remove_proc_entry("oplus_uprobe", proc_fs_uprobe);
	remove_proc_entry("uprobe_enable", proc_fs_uprobe_enable);
}

module_init(oplus_uprobe_init);
module_exit(oplus_uprobe_exit);

MODULE_IMPORT_NS(VFS_internal_I_am_really_a_filesystem_and_am_NOT_a_driver);
MODULE_AUTHOR("Cheng");
MODULE_DESCRIPTION("oplus uprobe driver");
MODULE_LICENSE("GPL v2");
