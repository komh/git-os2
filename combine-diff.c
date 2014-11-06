static int compare_paths(const struct combine_diff_path *one,
			  const struct diff_filespec *two)
{
	if (!S_ISDIR(one->mode) && !S_ISDIR(two->mode))
		return strcmp(one->path, two->path);

	return base_name_compare(one->path, strlen(one->path), one->mode,
				 two->path, strlen(two->path), two->mode);
}

	struct combine_diff_path *p, **tail = &curr;
	int i, cmp;
			hashcpy(p->oid.hash, q->queue[i]->two->sha1);
			hashcpy(p->parent[n].oid.hash, q->queue[i]->one->sha1);
		return curr;
	/*
	 * paths in curr (linked list) and q->queue[] (array) are
	 * both sorted in the tree order.
	 */
	i = 0;
	while ((p = *tail) != NULL) {
		cmp = ((i >= q->nr)
		       ? -1 : compare_paths(p, q->queue[i]->two));

		if (cmp < 0) {
			/* p->path not in q->queue[]; drop it */
			*tail = p->next;
			free(p);
		}
		if (cmp > 0) {
			/* q->queue[i] not in p->path; skip it */
			i++;
			continue;

		hashcpy(p->parent[n].oid.hash, q->queue[i]->one->sha1);
		p->parent[n].mode = q->queue[i]->one->mode;
		p->parent[n].status = q->queue[i]->status;

		tail = &p->next;
		i++;
static char *grab_blob(const struct object_id *oid, unsigned int mode,
				 "Subproject commit %s\n", oid_to_hex(oid));
	} else if (is_null_oid(oid)) {
		fill_filespec(df, oid->hash, 1, mode);
		blob = read_sha1_file(oid->hash, &type, size);
			die("object '%s' is not a blob!", oid_to_hex(oid));
static void combine_diff(const struct object_id *parent, unsigned int mode,
	const char *c_context = diff_get_color(use_color, DIFF_CONTEXT);
						    c_context, c_reset,
				fputs(c_context, stdout);
	int abbrev = DIFF_OPT_TST(opt, FULL_INDEX) ? GIT_SHA1_HEXSZ : DEFAULT_ABBREV;
		abb = find_unique_abbrev(elem->parent[i].oid.hash,
	abb = find_unique_abbrev(elem->oid.hash, abbrev);
		result = grab_blob(&elem->oid, elem->mode, &result_size,
			struct object_id oid;
			if (resolve_gitlink_ref(elem->path, "HEAD", oid.hash) < 0)
				result = grab_blob(&elem->oid, elem->mode,
				result = grab_blob(&oid, elem->mode,
			buf = grab_blob(&elem->parent[i].oid,
			if (!oidcmp(&elem->parent[i].oid,
				     &elem->parent[j].oid)) {
			combine_diff(&elem->parent[i].oid,
			printf(" %s", diff_unique_abbrev(p->parent[i].oid.hash,
		printf(" %s ", diff_unique_abbrev(p->oid.hash, opt->abbrev));
		hashcpy(pair->one[i].sha1, p->parent[i].oid.hash);
		pair->one[i].sha1_valid = !is_null_oid(&p->parent[i].oid);
	hashcpy(pair->two->sha1, p->oid.hash);
	pair->two->sha1_valid = !is_null_oid(&p->oid);
	for (i = 0, p = paths; p; p = p->next)
static const char *path_path(void *obj)
{
	struct combine_diff_path *path = (struct combine_diff_path *)obj;

	return path->path;
}


/* find set of paths that every parent touches */
static struct combine_diff_path *find_paths_generic(const unsigned char *sha1,
	const struct sha1_array *parents, struct diff_options *opt)
{
	struct combine_diff_path *paths = NULL;
	int i, num_parent = parents->nr;

	int output_format = opt->output_format;
	const char *orderfile = opt->orderfile;

	opt->output_format = DIFF_FORMAT_NO_OUTPUT;
	/* tell diff_tree to emit paths in sorted (=tree) order */
	opt->orderfile = NULL;

	/* D(A,P1...Pn) = D(A,P1) ^ ... ^ D(A,Pn)  (wrt paths) */
	for (i = 0; i < num_parent; i++) {
		/*
		 * show stat against the first parent even when doing
		 * combined diff.
		 */
		int stat_opt = (output_format &
				(DIFF_FORMAT_NUMSTAT|DIFF_FORMAT_DIFFSTAT));
		if (i == 0 && stat_opt)
			opt->output_format = stat_opt;
		else
			opt->output_format = DIFF_FORMAT_NO_OUTPUT;
		diff_tree_sha1(parents->sha1[i], sha1, "", opt);
		diffcore_std(opt);
		paths = intersect_paths(paths, i, num_parent);

		/* if showing diff, show it in requested order */
		if (opt->output_format != DIFF_FORMAT_NO_OUTPUT &&
		    orderfile) {
			diffcore_order(orderfile);
		}

		diff_flush(opt);
	}

	opt->output_format = output_format;
	opt->orderfile = orderfile;
	return paths;
}


/*
 * find set of paths that everybody touches, assuming diff is run without
 * rename/copy detection, etc, comparing all trees simultaneously (= faster).
 */
static struct combine_diff_path *find_paths_multitree(
	const unsigned char *sha1, const struct sha1_array *parents,
	struct diff_options *opt)
{
	int i, nparent = parents->nr;
	const unsigned char **parents_sha1;
	struct combine_diff_path paths_head;
	struct strbuf base;

	parents_sha1 = xmalloc(nparent * sizeof(parents_sha1[0]));
	for (i = 0; i < nparent; i++)
		parents_sha1[i] = parents->sha1[i];

	/* fake list head, so worker can assume it is non-NULL */
	paths_head.next = NULL;

	strbuf_init(&base, PATH_MAX);
	diff_tree_paths(&paths_head, sha1, parents_sha1, nparent, &base, opt);

	strbuf_release(&base);
	free(parents_sha1);
	return paths_head.next;
}


	struct combine_diff_path *p, *paths;
	int need_generic_pathscan;

	/* nothing to do, if no parents */
	if (!num_parent)
		return;

	show_log_first = !!rev->loginfo && !rev->no_commit_id;
	needsep = 0;
	if (show_log_first) {
		show_log(rev);

		if (rev->verbose_header && opt->output_format &&
		    opt->output_format != DIFF_FORMAT_NO_OUTPUT &&
		    !commit_format_is_empty(rev->commit_format))
			printf("%s%c", diff_line_prefix(opt),
			       opt->line_termination);
	}
	/* find set of paths that everybody touches
	 *
	 * NOTE
	 *
	 * Diffcore transformations are bound to diff_filespec and logic
	 * comparing two entries - i.e. they do not apply directly to combine
	 * diff.
	 *
	 * If some of such transformations is requested - we launch generic
	 * path scanning, which works significantly slower compared to
	 * simultaneous all-trees-in-one-go scan in find_paths_multitree().
	 *
	 * TODO some of the filters could be ported to work on
	 * combine_diff_paths - i.e. all functionality that skips paths, so in
	 * theory, we could end up having only multitree path scanning.
	 *
	 * NOTE please keep this semantically in sync with diffcore_std()
	 */
	need_generic_pathscan = opt->skip_stat_unmatch	||
			DIFF_OPT_TST(opt, FOLLOW_RENAMES)	||
			opt->break_opt != -1	||
			opt->detect_rename	||
			opt->pickaxe		||
			opt->filter;


	if (need_generic_pathscan) {
		/*
		 * NOTE generic case also handles --stat, as it computes
		 * diff(sha1,parent_i) for all i to do the job, specifically
		 * for parent0.
		 */
		paths = find_paths_generic(sha1, parents, &diffopts);
	}
	else {
		int stat_opt;
		paths = find_paths_multitree(sha1, parents, &diffopts);

		/*
		 * show stat against the first parent even
		stat_opt = (opt->output_format &
		if (stat_opt) {
			diff_tree_sha1(parents->sha1[0], sha1, "", &diffopts);
			diffcore_std(&diffopts);
			if (opt->orderfile)
				diffcore_order(opt->orderfile);
			diff_flush(&diffopts);
	/* find out number of surviving paths */
	for (num_paths = 0, p = paths; p; p = p->next)
		num_paths++;

	/* order paths according to diffcore_order */
	if (opt->orderfile && num_paths) {
		struct obj_order *o;

		o = xmalloc(sizeof(*o) * num_paths);
		for (i = 0, p = paths; p; p = p->next, i++)
			o[i].obj = p;
		order_objects(opt->orderfile, path_path, o, num_paths);
		for (i = 0; i < num_paths - 1; i++) {
			p = o[i].obj;
			p->next = o[i+1].obj;
		}

		p = o[num_paths-1].obj;
		p->next = NULL;
		paths = o[0].obj;
		free(o);


			for (p = paths; p; p = p->next)
				show_raw_diff(p, num_parent, rev);
			for (p = paths; p; p = p->next)
				show_patch_diff(p, num_parent, dense,
						0, rev);