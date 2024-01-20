#include <linux/module.h>
#include <linux/types.h>
#include <linux/skbuff.h>
#include <linux/filter.h>
#include <linux/idr.h>
#include <linux/udp.h>
#include <linux/version.h>

#include <net/rtnetlink.h>
#include <net/pkt_cls.h>
#include <net/sock.h>

#include "../include/dpi_api.h"
#include "../include/comm_def.h"
#include "cls_dpi.h"


#define LOG_TAG "dpi_cls"

#define logt(fmt, args...) LOG(LOG_TAG, fmt, ##args)


enum {
	TCA_DPI_UNSPEC,
	TCA_DPI_ACT,
	TCA_DPI_POLICE,
	TCA_DPI_TYPE,
	TCA_DPI_ID,
	TCA_DPI_MARK,
	TCA_DPI_CLASSID,
	TCA_DPI_PAD,
	__TCA_DPI_MAX,
};

#define TCA_DPI_MAX (__TCA_DPI_MAX - 1)


struct cls_dpi_head {
	struct list_head plist;
	struct idr handle_idr;
	struct rcu_head rcu;
};

struct cls_dpi_data {
	struct list_head link;
	struct tcf_result res;
	struct tcf_exts exts;
	struct tcf_proto *tp;
	struct rcu_work rwork;
	u32 handle;
	u32 type;
	u64 dpi_id;
	u32 mark;
};


static const struct nla_policy dpi_policy[TCA_DPI_MAX + 1] = {
	[TCA_DPI_TYPE]		= { .type = NLA_U32 },
	[TCA_DPI_ID]		= { .type = NLA_U64 },
	[TCA_DPI_MARK]		= { .type = NLA_U32 },
	[TCA_DPI_CLASSID]	= { .type = NLA_U32 },
};

static void __cls_dpi_delete_data(struct cls_dpi_data *pdata)
{
	tcf_exts_destroy(&pdata->exts);
	tcf_exts_put_net(&pdata->exts);
	kfree(pdata);
}

static void cls_dpi_delete_work(struct work_struct *work)
{
	struct cls_dpi_data *pdata = container_of(to_rcu_work(work), struct cls_dpi_data, rwork);
	rtnl_lock();
	__cls_dpi_delete_data(pdata);
	rtnl_unlock();
}

static void __cls_dpi_delete(struct tcf_proto *tp, struct cls_dpi_data *prog, struct netlink_ext_ack *extack)
{
	struct cls_dpi_head *head = rtnl_dereference(tp->root);

	idr_remove(&head->handle_idr, prog->handle);
	list_del_rcu(&prog->link);
	tcf_unbind_filter(tp, &prog->res);
	if (tcf_exts_get_net(&prog->exts)) {
		tcf_queue_work(&prog->rwork, cls_dpi_delete_work);
	} else {
		__cls_dpi_delete_data(prog);
	}
}


static int check_dpi_id_valid(u32 type, u64 dpi_id)
{
	u64 mask = 0;
	switch (type) {
	case DPI_TYPE_UID:
		mask = DPI_ID_UID_MASK;
		break;
	case DPI_TYPE_APP:
		mask = DPI_ID_APP_MASK;
		break;
	case DPI_TYPE_FUNC:
		mask = DPI_ID_FUNC_MASK;
		break;
	case DPI_TYPE_STREAM:
		mask = DPI_IP_STREAM_MASK;
		break;
	default:
		return 0;
	}

	if ((dpi_id & mask) != dpi_id) {
		logt("dpi_id invalid");
		return 0;
	}
	return 1;
}

static int check_mark_valid(u32 mark)
{
	if (mark & ~LIMIT_REDIRECT_MARK_MASK) {
		logt("mark invalid %u", mark);
		return 0;
	}
	return 1;
}


static bool match_skb_dpi(struct sk_buff *skb, bool ingress, struct cls_dpi_data *pdata)
{
	u64 dpi_id = get_skb_dpi_id(skb, ingress?0:1, ingress?1:0);
	u64 mask = 0;

	if (dpi_id == 0) {
		return 0;
	}

	switch (pdata->type) {
	case DPI_TYPE_UID:
		mask = DPI_ID_UID_MASK;
		break;
	case DPI_TYPE_APP:
		mask = DPI_ID_APP_MASK;
		break;
	case DPI_TYPE_FUNC:
		mask = DPI_ID_FUNC_MASK;
		break;
	case DPI_TYPE_STREAM:
		mask = DPI_IP_STREAM_MASK;
		break;
	default:
		return 0;
	}

	if ((dpi_id & mask) == pdata->dpi_id) {
		if (pdata->mark) {
			skb->mark = (skb->mark | pdata->mark);
		}
		return 1;
	}
	return 0;
}

static int cls_dpi_classify(struct sk_buff *skb, const struct tcf_proto *tp, struct tcf_result *res)
{
	struct cls_dpi_head *head = rcu_dereference_bh(tp->root);
	bool at_ingress = skb_at_tc_ingress(skb);
	struct cls_dpi_data *pdata;
	int ret = -1;

	rcu_read_lock();
	list_for_each_entry_rcu(pdata, &head->plist, link) {
		int filter_res;

		if (at_ingress) {
			__skb_push(skb, skb->mac_len);
			filter_res = match_skb_dpi(skb, at_ingress, pdata);
			__skb_pull(skb, skb->mac_len);
		} else {
			filter_res = match_skb_dpi(skb, at_ingress, pdata);
		}
		if (!filter_res) {
			continue;
		}
		if (pdata->res.classid) {
			res->class = 0;
			res->classid = pdata->res.classid;
		}
		ret = tcf_exts_exec(skb, &pdata->exts, res);
		if (ret < 0) {
			continue;
		}
		break;
	}
	rcu_read_unlock();

	return ret;
}

static int cls_dpi_init(struct tcf_proto *tp)
{
	struct cls_dpi_head *head;

	head = kzalloc(sizeof(struct cls_dpi_head), GFP_KERNEL);
	if (head == NULL) {
		return -ENOBUFS;
	}

	INIT_LIST_HEAD_RCU(&head->plist);
	idr_init(&head->handle_idr);
	rcu_assign_pointer(tp->root, head);

	return 0;
}

static void *cls_dpi_get(struct tcf_proto *tp, u32 handle)
{
	struct cls_dpi_head *head = rtnl_dereference(tp->root);
	struct cls_dpi_data *data = NULL;

	list_for_each_entry(data, &head->plist, link) {
		if (data->handle == handle) {
			return data;
		}
	}
	return NULL;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0))
static int cls_dpi_set_params(struct net *net, struct tcf_proto *tp, struct cls_dpi_data *pdata, unsigned long base,
								struct nlattr **tb, struct nlattr *est, u32 flags, struct netlink_ext_ack *extack)
{
	int ret = 0;

	ret = tcf_exts_validate(net, tp, tb, est, &pdata->exts, flags, extack);
	if (ret < 0) {
		return ret;
	}

	if (tb[TCA_DPI_TYPE]) {
		pdata->type = nla_get_u32(tb[TCA_DPI_TYPE]);
	}

	if (tb[TCA_DPI_ID]) {
		pdata->dpi_id = nla_get_u64(tb[TCA_DPI_ID]);
	}

	if (tb[TCA_DPI_MARK]) {
		pdata->mark = nla_get_u32(tb[TCA_DPI_MARK]);
	}

	if (!check_dpi_id_valid(pdata->type, pdata->dpi_id)) {
		return -EINVAL;
	}

	if (pdata->mark && !check_mark_valid(pdata->mark)) {
		return EINVAL;
	}

	if (tb[TCA_DPI_CLASSID]) {
		pdata->res.classid = nla_get_u32(tb[TCA_DPI_CLASSID]);
		tcf_bind_filter(tp, &pdata->res, base);
	}

	return 0;
}


static int cls_dpi_change(struct net *net, struct sk_buff *in_skb, struct tcf_proto *tp, unsigned long base,
						u32 handle, struct nlattr **tca, void **arg, u32 flags, struct netlink_ext_ack *extack)
{
	int ret = 0;
	struct nlattr *tb[TCA_DPI_MAX + 1];
	struct cls_dpi_head *head = rtnl_dereference(tp->root);
	struct cls_dpi_data *old = *arg;
	struct cls_dpi_data *new_data = NULL;

	if (!tca[TCA_OPTIONS]) {
		return -EINVAL;
	}

	ret = nla_parse_nested_deprecated(tb, TCA_DPI_MAX, tca[TCA_OPTIONS], dpi_policy, NULL);
	if (ret < 0) {
		return ret;
	}

	new_data = kzalloc(sizeof(*new_data), GFP_KERNEL);
	if (!new_data) {
		return ENOBUFS;
	}

	ret = tcf_exts_init(&new_data->exts, net, TCA_DPI_ACT, TCA_DPI_POLICE);
	if (ret < 0) {
		goto errout;
	}

	if (old) {
		if (handle && old->handle != handle) {
			ret = -EINVAL;
			goto errout;
		}
	}
	if (handle == 0) {
		handle = 1;
		ret = idr_alloc_u32(&head->handle_idr, new_data, &handle, INT_MAX, GFP_KERNEL);
	} else if (!old) {
		ret = idr_alloc_u32(&head->handle_idr, new_data, &handle, handle, GFP_KERNEL);
	}
	if (ret) {
		goto errout;
	}
	new_data->handle = handle;

	ret = cls_dpi_set_params(net, tp, new_data, base, tb, tca[TCA_RATE], flags, extack);
	if (ret < 0) {
		goto errout_idr;
	}

	if (old) {
		logt("replace filter!");
		idr_replace(&head->handle_idr, new_data, handle);
		list_replace_rcu(&old->link, &new_data->link);
		tcf_unbind_filter(tp, &old->res);
		tcf_exts_get_net(&old->exts);
		tcf_queue_work(&old->rwork, cls_dpi_delete_work);
	} else {
		logt("add filter!");
		list_add_rcu(&new_data->link, &head->plist);
	}
	*arg = new_data;
	return 0;

errout_idr:
	if (!old) {
		idr_remove(&head->handle_idr, new_data->handle);
	}
errout:
	tcf_exts_destroy(&new_data->exts);
	kfree(new_data);

	return ret;
}
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
static int cls_dpi_set_params(struct net *net, struct tcf_proto *tp, struct cls_dpi_data *pdata, unsigned long base,
								struct nlattr **tb, struct nlattr *est, bool ovr, struct netlink_ext_ack *extack)
{
	int ret = 0;

	ret = tcf_exts_validate(net, tp, tb, est, &pdata->exts, ovr, true, extack);
	if (ret < 0) {
		return ret;
	}

	if (tb[TCA_DPI_TYPE]) {
		pdata->type = nla_get_u32(tb[TCA_DPI_TYPE]);
	}

	if (tb[TCA_DPI_ID]) {
		pdata->dpi_id = nla_get_u64(tb[TCA_DPI_ID]);
	}

	if (tb[TCA_DPI_MARK]) {
		pdata->mark = nla_get_u32(tb[TCA_DPI_MARK]);
	}

	if (!check_dpi_id_valid(pdata->type, pdata->dpi_id)) {
		return -EINVAL;
	}

	if (pdata->mark && !check_mark_valid(pdata->mark)) {
		return EINVAL;
	}

	if (tb[TCA_DPI_CLASSID]) {
		pdata->res.classid = nla_get_u32(tb[TCA_DPI_CLASSID]);
		tcf_bind_filter(tp, &pdata->res, base);
	}

	return 0;
}



static int cls_dpi_change(struct net *net, struct sk_buff *in_skb, struct tcf_proto *tp, unsigned long base,
						u32 handle, struct nlattr **tca, void **arg, bool ovr, bool rtnl_held, struct netlink_ext_ack *extack)
{
	int ret = 0;
	struct nlattr *tb[TCA_DPI_MAX + 1];
	struct cls_dpi_head *head = rtnl_dereference(tp->root);
	struct cls_dpi_data *old = *arg;
	struct cls_dpi_data *new_data = NULL;

	if (!tca[TCA_OPTIONS]) {
		return -EINVAL;
	}

	ret = nla_parse_nested_deprecated(tb, TCA_DPI_MAX, tca[TCA_OPTIONS], dpi_policy, NULL);
	if (ret < 0) {
		return ret;
	}

	new_data = kzalloc(sizeof(*new_data), GFP_KERNEL);
	if (!new_data) {
		return ENOBUFS;
	}

	ret = tcf_exts_init(&new_data->exts, net, TCA_DPI_ACT, TCA_DPI_POLICE);
	if (ret < 0) {
		goto errout;
	}

	if (old) {
		if (handle && old->handle != handle) {
			ret = -EINVAL;
			goto errout;
		}
	}
	if (handle == 0) {
		handle = 1;
		ret = idr_alloc_u32(&head->handle_idr, new_data, &handle, INT_MAX, GFP_KERNEL);
	} else if (!old) {
		ret = idr_alloc_u32(&head->handle_idr, new_data, &handle, handle, GFP_KERNEL);
	}
	if (ret) {
		goto errout;
	}
	new_data->handle = handle;

	ret = cls_dpi_set_params(net, tp, new_data, base, tb, tca[TCA_RATE], ovr, extack);
	if (ret < 0) {
		goto errout_idr;
	}

	if (old) {
		logt("replace filter!");
		idr_replace(&head->handle_idr, new_data, handle);
		list_replace_rcu(&old->link, &new_data->link);
		tcf_unbind_filter(tp, &old->res);
		tcf_exts_get_net(&old->exts);
		tcf_queue_work(&old->rwork, cls_dpi_delete_work);
	} else {
		logt("add filter!");
		list_add_rcu(&new_data->link, &head->plist);
	}
	*arg = new_data;
	return 0;

errout_idr:
	if (!old) {
		idr_remove(&head->handle_idr, new_data->handle);
	}
errout:
	tcf_exts_destroy(&new_data->exts);
	kfree(new_data);

	return ret;
}
#else
static int cls_dpi_set_params(struct net *net, struct tcf_proto *tp, struct cls_dpi_data *pdata, unsigned long base,
								struct nlattr **tb, struct nlattr *est, bool ovr, struct netlink_ext_ack *extack)
{
	int ret = 0;

	ret = tcf_exts_validate(net, tp, tb, est, &pdata->exts, ovr, extack);
	if (ret < 0) {
		return ret;
	}

	if (tb[TCA_DPI_TYPE]) {
		pdata->type = nla_get_u32(tb[TCA_DPI_TYPE]);
	}

	if (tb[TCA_DPI_ID]) {
		pdata->dpi_id = nla_get_u64(tb[TCA_DPI_ID]);
	}

	if (tb[TCA_DPI_MARK]) {
		pdata->mark = nla_get_u32(tb[TCA_DPI_MARK]);
	}

	if (!check_dpi_id_valid(pdata->type, pdata->dpi_id)) {
		return -EINVAL;
	}

	if (pdata->mark && !check_mark_valid(pdata->mark)) {
		return EINVAL;
	}

	if (tb[TCA_DPI_CLASSID]) {
		pdata->res.classid = nla_get_u32(tb[TCA_DPI_CLASSID]);
		tcf_bind_filter(tp, &pdata->res, base);
	}

	return 0;
}

static int cls_dpi_change(struct net *net, struct sk_buff *in_skb, struct tcf_proto *tp, unsigned long base,
						u32 handle, struct nlattr **tca, void **arg, bool ovr, struct netlink_ext_ack *extack)
{
	int ret = 0;
	struct nlattr *tb[TCA_DPI_MAX + 1];
	struct cls_dpi_head *head = rtnl_dereference(tp->root);
	struct cls_dpi_data *old = *arg;
	struct cls_dpi_data *new_data = NULL;

	if (!tca[TCA_OPTIONS]) {
		return -EINVAL;
	}

	ret = nla_parse_nested(tb, TCA_DPI_MAX, tca[TCA_OPTIONS], dpi_policy, NULL);
	if (ret < 0) {
		return ret;
	}

	new_data = kzalloc(sizeof(*new_data), GFP_KERNEL);
	if (!new_data) {
		return ENOBUFS;
	}

	ret = tcf_exts_init(&new_data->exts, TCA_DPI_ACT, TCA_DPI_POLICE);
	if (ret < 0) {
		goto errout;
	}

	if (old) {
		if (handle && old->handle != handle) {
			ret = -EINVAL;
			goto errout;
		}
	}
	if (handle == 0) {
		handle = 1;
		ret = idr_alloc_u32(&head->handle_idr, new_data, &handle, INT_MAX, GFP_KERNEL);
	} else if (!old) {
		ret = idr_alloc_u32(&head->handle_idr, new_data, &handle, handle, GFP_KERNEL);
	}
	if (ret) {
		goto errout;
	}
	new_data->handle = handle;

	ret = cls_dpi_set_params(net, tp, new_data, base, tb, tca[TCA_RATE], ovr, extack);
	if (ret < 0) {
		goto errout_idr;
	}

	if (old) {
		logt("replace filter!");
		idr_replace(&head->handle_idr, new_data, handle);
		list_replace_rcu(&old->link, &new_data->link);
		tcf_unbind_filter(tp, &old->res);
		tcf_exts_get_net(&old->exts);
		tcf_queue_work(&old->rwork, cls_dpi_delete_work);
	} else {
		logt("add filter!");
		list_add_rcu(&new_data->link, &head->plist);
	}
	*arg = new_data;
	return 0;

errout_idr:
	if (!old) {
		idr_remove(&head->handle_idr, new_data->handle);
	}
errout:
	tcf_exts_destroy(&new_data->exts);
	kfree(new_data);

	return ret;
}
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
static void cls_dpi_destroy(struct tcf_proto *tp, bool rtnl_held, struct netlink_ext_ack *extack)
{
	struct cls_dpi_head *head = rtnl_dereference(tp->root);
	struct cls_dpi_data *prog, *tmp;

	list_for_each_entry_safe(prog, tmp, &head->plist, link) {
		__cls_dpi_delete(tp, prog, extack);
	}

	idr_destroy(&head->handle_idr);
	kfree_rcu(head, rcu);
}

static int cls_dpi_delete(struct tcf_proto *tp, void *arg, bool *last, bool rtnl_held, struct netlink_ext_ack *extack)
{
	struct cls_dpi_head *head = rtnl_dereference(tp->root);
	__cls_dpi_delete(tp, arg, extack);
	*last = list_empty(&head->plist);
	return 0;
}

static void cls_dpi_walk(struct tcf_proto *tp, struct tcf_walker *arg, bool rtnl_held)
{
	struct cls_dpi_head *head = rtnl_dereference(tp->root);
	struct cls_dpi_data *prog;

	list_for_each_entry(prog, &head->plist, link) {
		if (arg->count < arg->skip) {
			goto skip;
		}
		if (arg->fn(tp, prog, arg) < 0) {
			arg->stop = 1;
			break;
		}
skip:
		arg->count++;
	}
}

static int cls_dpi_dump(struct net *net, struct tcf_proto *tp, void *fh, struct sk_buff *skb, struct tcmsg *tm, bool rtnl_held)
{
	struct cls_dpi_data *prog = fh;
	struct nlattr *nest;

	if (prog == NULL) {
		return skb->len;
	}

	tm->tcm_handle = prog->handle;
	nest = nla_nest_start_noflag(skb, TCA_OPTIONS);
	if (nest == NULL) {
		goto nla_put_failure;
	}
	if (prog->res.classid && nla_put_u32(skb, TCA_DPI_CLASSID, prog->res.classid)) {
		goto nla_put_failure;
	}
	if (nla_put_u32(skb, TCA_DPI_TYPE, prog->type)) {
		goto nla_put_failure;
	}
	if (nla_put_u64_64bit(skb, TCA_DPI_ID, prog->dpi_id, TCA_DPI_PAD)) {
		goto nla_put_failure;
	}
	if (nla_put_u32(skb, TCA_DPI_MARK, prog->mark)) {
		goto nla_put_failure;
	}
	if (tcf_exts_dump(skb, &prog->exts) < 0) {
		goto nla_put_failure;
	}
	nla_nest_end(skb, nest);

	if (tcf_exts_dump_stats(skb, &prog->exts) < 0) {
		goto nla_put_failure;
	}
	return skb->len;

nla_put_failure:
	nla_nest_cancel(skb, nest);
	return -1;
}
#else
static void cls_dpi_destroy(struct tcf_proto *tp, struct netlink_ext_ack *extack)
{
	struct cls_dpi_head *head = rtnl_dereference(tp->root);
	struct cls_dpi_data *prog, *tmp;

	list_for_each_entry_safe(prog, tmp, &head->plist, link) {
		__cls_dpi_delete(tp, prog, extack);
	}

	idr_destroy(&head->handle_idr);
	kfree_rcu(head, rcu);
}

static int cls_dpi_delete(struct tcf_proto *tp, void *arg, bool *last, struct netlink_ext_ack *extack)
{
	struct cls_dpi_head *head = rtnl_dereference(tp->root);
	__cls_dpi_delete(tp, arg, extack);
	*last = list_empty(&head->plist);
	return 0;
}

static void cls_dpi_walk(struct tcf_proto *tp, struct tcf_walker *arg)
{
	struct cls_dpi_head *head = rtnl_dereference(tp->root);
	struct cls_dpi_data *prog;

	list_for_each_entry(prog, &head->plist, link) {
		if (arg->count < arg->skip) {
			goto skip;
		}
		if (arg->fn(tp, prog, arg) < 0) {
			arg->stop = 1;
			break;
		}
skip:
		arg->count++;
	}
}

static int cls_dpi_dump(struct net *net, struct tcf_proto *tp, void *fh, struct sk_buff *skb, struct tcmsg *tm)
{
	struct cls_dpi_data *prog = fh;
	struct nlattr *nest;

	if (prog == NULL) {
		return skb->len;
	}

	tm->tcm_handle = prog->handle;
	nest = nla_nest_start(skb, TCA_OPTIONS);
	if (nest == NULL) {
		goto nla_put_failure;
	}
	if (prog->res.classid && nla_put_u32(skb, TCA_DPI_CLASSID, prog->res.classid)) {
		goto nla_put_failure;
	}
	if (nla_put_u32(skb, TCA_DPI_TYPE, prog->type)) {
		goto nla_put_failure;
	}
	if (nla_put_u64_64bit(skb, TCA_DPI_ID, prog->dpi_id, TCA_DPI_PAD)) {
		goto nla_put_failure;
	}
	if (nla_put_u32(skb, TCA_DPI_MARK, prog->mark)) {
		goto nla_put_failure;
	}
	if (tcf_exts_dump(skb, &prog->exts) < 0) {
		goto nla_put_failure;
	}
	nla_nest_end(skb, nest);

	if (tcf_exts_dump_stats(skb, &prog->exts) < 0) {
		goto nla_put_failure;
	}
	return skb->len;

nla_put_failure:
	nla_nest_cancel(skb, nest);
	return -1;
}
#endif

static void cls_dpi_bind_class(void *fh, u32 classid, unsigned long cl, void *q, unsigned long base)
{
	struct cls_dpi_data *prog = fh;

	if (prog && prog->res.classid == classid) {
		if (cl) {
			__tcf_bind_filter(q, &prog->res, base);
		} else {
			__tcf_unbind_filter(q, &prog->res);
		}
	}
}

static struct tcf_proto_ops cls_dpi_ops __read_mostly = {
	.kind = "dpi",
	.owner		=	THIS_MODULE,
	.classify	=	cls_dpi_classify,
	.init		=	cls_dpi_init,
	.destroy	=	cls_dpi_destroy,
	.get		=	cls_dpi_get,
	.change		=	cls_dpi_change,
	.delete		=	cls_dpi_delete,
	.walk		=	cls_dpi_walk,
	.reoffload	=	NULL,
	.dump		=	cls_dpi_dump,
	.bind_class	=	cls_dpi_bind_class,
};


int cls_dpi_mod_init(void)
{
	printk("cls_dpi_mod_init\n");
	return register_tcf_proto_ops(&cls_dpi_ops);
}

void cls_dpi_mod_exit(void)
{
	printk("cls_dpi_mod_exit\n");
	unregister_tcf_proto_ops(&cls_dpi_ops);
}


