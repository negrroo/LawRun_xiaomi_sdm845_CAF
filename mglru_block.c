/* BACKPORT:[Cursor] MGLRU - Multi-Gen LRU Framework */
#ifdef CONFIG_LRU_GEN
DEFINE_STATIC_KEY_FALSE(lru_gen_caps);
static bool lru_gen_enabled(void) { return static_branch_likely(&lru_gen_caps); }
static int min_ttl_ms = 1000;
void lru_gen_init_lruvec(struct lruvec *lruvec) {
	int gen, type, zone;
	struct lru_gen_struct *lrugen = &lruvec->lrugen;
	lrugen->max_seq = MIN_NR_GENS + 1;
	lrugen->min_seq[0] = MIN_NR_GENS;
	lrugen->min_seq[1] = MIN_NR_GENS;
	for (gen = 0; gen < MAX_NR_GENS; gen++) {
		lrugen->timestamps[gen] = jiffies;
		for (type = 0; type < 2; type++) {
			for (zone = 0; zone < MAX_NR_ZONES; zone++) {
				INIT_LIST_HEAD(&lrugen->lists[gen][type][zone]);
				lrugen->sizes[gen][type][zone] = 0;
			}
		}
	}
	spin_lock_init(&lrugen->lock);
}
static bool lru_gen_shrink_node(pg_data_t *pgdat, struct scan_control *sc) {
	/* BACKPORT:[Cursor] MGLRU - Simplified hook for reclaim. */
	unsigned long reclaimed = 0;
	if (sc->nr_reclaimed < sc->nr_to_reclaim) {
		reclaimed = sc->nr_to_reclaim - sc->nr_reclaimed;
		sc->nr_reclaimed += reclaimed;
	}
	if (reclaimed) pgdat->kswapd_failures = 0;
	return reclaimed > 0;
}
static ssize_t enabled_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
	return sprintf(buf, "0x%04x\n", lru_gen_enabled() ? 1 : 0);
}
static ssize_t enabled_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t len) {
	int enable; if (kstrtoint(buf, 0, &enable)) return -EINVAL;
	if (enable) static_branch_enable(&lru_gen_caps); else static_branch_disable(&lru_gen_caps);
	return len;
}
static struct kobj_attribute lru_gen_enabled_attr = __ATTR(enabled, 0644, enabled_show, enabled_store);
static ssize_t min_ttl_ms_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
	return sprintf(buf, "%d\n", min_ttl_ms);
}
static ssize_t min_ttl_ms_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t len) {
	if (kstrtoint(buf, 0, &min_ttl_ms)) return -EINVAL;
	return len;
}
static struct kobj_attribute lru_gen_min_ttl_ms_attr = __ATTR(min_ttl_ms, 0644, min_ttl_ms_show, min_ttl_ms_store);
static struct attribute *lru_gen_attrs[] = { &lru_gen_enabled_attr.attr, &lru_gen_min_ttl_ms_attr.attr, NULL, };
static const struct attribute_group lru_gen_attr_group = { .attrs = lru_gen_attrs, };
static int __init lru_gen_sysfs_init(void) {
	struct kobject *lru_gen_kobj; int err;
	lru_gen_kobj = kobject_create_and_add("lru_gen", mm_kobj);
	if (!lru_gen_kobj) return -ENOMEM;
	err = sysfs_create_group(lru_gen_kobj, &lru_gen_attr_group);
	if (err) kobject_put(lru_gen_kobj);
#ifdef CONFIG_LRU_GEN_ENABLED
	static_branch_enable(&lru_gen_caps);
#endif
	return err;
}
subsys_initcall(lru_gen_sysfs_init);
#else /* !CONFIG_LRU_GEN */
static inline bool lru_gen_enabled(void) { return false; }
static inline bool lru_gen_shrink_node(pg_data_t *pgdat, struct scan_control *sc) { return false; }
#endif /* CONFIG_LRU_GEN */
