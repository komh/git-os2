#include "xdiff/xmacros.h"
#include "userdiff.h"
#include "sha1-array.h"
#include "revision.h"
	struct lline *next, *prev;
/* Lines lost from current parent (before coalescing) */
struct plost {
	struct lline *lost_head, *lost_tail;
	int len;
};

	/* Accumulated and coalesced lost lines */
	struct lline *lost;
	int lenlost;
	struct plost plost;
static int match_string_spaces(const char *line1, int len1,
			       const char *line2, int len2,
			       long flags)
{
	if (flags & XDF_WHITESPACE_FLAGS) {
		for (; len1 > 0 && XDL_ISSPACE(line1[len1 - 1]); len1--);
		for (; len2 > 0 && XDL_ISSPACE(line2[len2 - 1]); len2--);
	}

	if (!(flags & (XDF_IGNORE_WHITESPACE | XDF_IGNORE_WHITESPACE_CHANGE)))
		return (len1 == len2 && !memcmp(line1, line2, len1));

	while (len1 > 0 && len2 > 0) {
		len1--;
		len2--;
		if (XDL_ISSPACE(line1[len1]) || XDL_ISSPACE(line2[len2])) {
			if ((flags & XDF_IGNORE_WHITESPACE_CHANGE) &&
			    (!XDL_ISSPACE(line1[len1]) || !XDL_ISSPACE(line2[len2])))
				return 0;

			for (; len1 > 0 && XDL_ISSPACE(line1[len1]); len1--);
			for (; len2 > 0 && XDL_ISSPACE(line2[len2]); len2--);
		}
		if (line1[len1] != line2[len2])
			return 0;
	}

	if (flags & XDF_IGNORE_WHITESPACE) {
		/* Consume remaining spaces */
		for (; len1 > 0 && XDL_ISSPACE(line1[len1 - 1]); len1--);
		for (; len2 > 0 && XDL_ISSPACE(line2[len2 - 1]); len2--);
	}

	/* We matched full line1 and line2 */
	if (!len1 && !len2)
		return 1;

	return 0;
}

enum coalesce_direction { MATCH, BASE, NEW };

/* Coalesce new lines into base by finding LCS */
static struct lline *coalesce_lines(struct lline *base, int *lenbase,
				    struct lline *new, int lennew,
				    unsigned long parent, long flags)
{
	int **lcs;
	enum coalesce_direction **directions;
	struct lline *baseend, *newend = NULL;
	int i, j, origbaselen = *lenbase;

	if (new == NULL)
		return base;

	if (base == NULL) {
		*lenbase = lennew;
		return new;
	}

	/*
	 * Coalesce new lines into base by finding the LCS
	 * - Create the table to run dynamic programming
	 * - Compute the LCS
	 * - Then reverse read the direction structure:
	 *   - If we have MATCH, assign parent to base flag, and consume
	 *   both baseend and newend
	 *   - Else if we have BASE, consume baseend
	 *   - Else if we have NEW, insert newend lline into base and
	 *   consume newend
	 */
	lcs = xcalloc(origbaselen + 1, sizeof(int*));
	directions = xcalloc(origbaselen + 1, sizeof(enum coalesce_direction*));
	for (i = 0; i < origbaselen + 1; i++) {
		lcs[i] = xcalloc(lennew + 1, sizeof(int));
		directions[i] = xcalloc(lennew + 1, sizeof(enum coalesce_direction));
		directions[i][0] = BASE;
	}
	for (j = 1; j < lennew + 1; j++)
		directions[0][j] = NEW;

	for (i = 1, baseend = base; i < origbaselen + 1; i++) {
		for (j = 1, newend = new; j < lennew + 1; j++) {
			if (match_string_spaces(baseend->line, baseend->len,
						newend->line, newend->len, flags)) {
				lcs[i][j] = lcs[i - 1][j - 1] + 1;
				directions[i][j] = MATCH;
			} else if (lcs[i][j - 1] >= lcs[i - 1][j]) {
				lcs[i][j] = lcs[i][j - 1];
				directions[i][j] = NEW;
			} else {
				lcs[i][j] = lcs[i - 1][j];
				directions[i][j] = BASE;
			}
			if (newend->next)
				newend = newend->next;
		}
		if (baseend->next)
			baseend = baseend->next;
	}

	for (i = 0; i < origbaselen + 1; i++)
		free(lcs[i]);
	free(lcs);

	/* At this point, baseend and newend point to the end of each lists */
	i--;
	j--;
	while (i != 0 || j != 0) {
		if (directions[i][j] == MATCH) {
			baseend->parent_map |= 1<<parent;
			baseend = baseend->prev;
			newend = newend->prev;
			i--;
			j--;
		} else if (directions[i][j] == NEW) {
			struct lline *lline;

			lline = newend;
			/* Remove lline from new list and update newend */
			if (lline->prev)
				lline->prev->next = lline->next;
			else
				new = lline->next;
			if (lline->next)
				lline->next->prev = lline->prev;

			newend = lline->prev;
			j--;

			/* Add lline to base list */
			if (baseend) {
				lline->next = baseend->next;
				lline->prev = baseend;
				if (lline->prev)
					lline->prev->next = lline;
			}
			else {
				lline->next = base;
				base = lline;
			}
			(*lenbase)++;

			if (lline->next)
				lline->next->prev = lline;

		} else {
			baseend = baseend->prev;
			i--;
		}
	}

	newend = new;
	while (newend) {
		struct lline *lline = newend;
		newend = newend->next;
		free(lline);
	}

	for (i = 0; i < origbaselen + 1; i++)
		free(directions[i]);
	free(directions);

	return base;
}

static char *grab_blob(const unsigned char *sha1, unsigned int mode,
		       unsigned long *size, struct userdiff_driver *textconv,
		       const char *path)
	} else if (textconv) {
		struct diff_filespec *df = alloc_filespec(path);
		fill_filespec(df, sha1, 1, mode);
		*size = fill_textconv(textconv, df, &blob);
		free_filespec(df);
	lline->prev = sline->plost.lost_tail;
	if (lline->prev)
		lline->prev->next = lline;
	else
		sline->plost.lost_head = lline;
	sline->plost.lost_tail = lline;
	sline->plost.len++;
			 int num_parent, int result_deleted,
			 struct userdiff_driver *textconv,
			 const char *path, long flags)
	parent_file.ptr = grab_blob(parent, mode, &sz, textconv, path);
	xpp.flags = flags;
		/* Coalesce new lines */
		if (sline[lno].plost.lost_head) {
			struct sline *sl = &sline[lno];
			sl->lost = coalesce_lines(sl->lost, &sl->lenlost,
						  sl->plost.lost_head,
						  sl->plost.len, n, flags);
			sl->plost.lost_head = sl->plost.lost_tail = NULL;
			sl->plost.len = 0;
		}

		ll = sline[lno].lost;
	return ((sline->flag & all_mask) || sline->lost);
		while (j < i) {
			if (!(sline[j].flag & mark))
				sline[j].flag |= no_pre_delete;
			sline[j++].flag |= mark;
		}
				while (la && j <= --la) {
			struct lline *ll = sline[j].lost;
static void dump_sline(struct sline *sline, const char *line_prefix,
		       unsigned long cnt, int num_parent,
		printf("%s%s", line_prefix, c_frag);
			ll = (sl->flag & no_pre_delete) ? NULL : sl->lost;
				printf("%s%s", line_prefix, c_old);
			fputs(line_prefix, stdout);
		struct lline *ll = sline->lost;
			     const char *line_prefix,
	strbuf_addstr(&buf, line_prefix);
static void show_combined_header(struct combine_diff_path *elem,
				 int num_parent,
				 int dense,
				 struct rev_info *rev,
				 const char *line_prefix,
				 int mode_differs,
				 int show_file_header)
{
	struct diff_options *opt = &rev->diffopt;
	int abbrev = DIFF_OPT_TST(opt, FULL_INDEX) ? 40 : DEFAULT_ABBREV;
	const char *a_prefix = opt->a_prefix ? opt->a_prefix : "a/";
	const char *b_prefix = opt->b_prefix ? opt->b_prefix : "b/";
	const char *c_meta = diff_get_color_opt(opt, DIFF_METAINFO);
	const char *c_reset = diff_get_color_opt(opt, DIFF_RESET);
	const char *abb;
	int added = 0;
	int deleted = 0;
	int i;

	if (rev->loginfo && !rev->no_commit_id)
		show_log(rev);

	dump_quoted_path(dense ? "diff --cc " : "diff --combined ",
			 "", elem->path, line_prefix, c_meta, c_reset);
	printf("%s%sindex ", line_prefix, c_meta);
	for (i = 0; i < num_parent; i++) {
		abb = find_unique_abbrev(elem->parent[i].sha1,
					 abbrev);
		printf("%s%s", i ? "," : "", abb);
	}
	abb = find_unique_abbrev(elem->sha1, abbrev);
	printf("..%s%s\n", abb, c_reset);

	if (mode_differs) {
		deleted = !elem->mode;

		/* We say it was added if nobody had it */
		added = !deleted;
		for (i = 0; added && i < num_parent; i++)
			if (elem->parent[i].status !=
			    DIFF_STATUS_ADDED)
				added = 0;
		if (added)
			printf("%s%snew file mode %06o",
			       line_prefix, c_meta, elem->mode);
		else {
			if (deleted)
				printf("%s%sdeleted file ",
				       line_prefix, c_meta);
			printf("mode ");
			for (i = 0; i < num_parent; i++) {
				printf("%s%06o", i ? "," : "",
				       elem->parent[i].mode);
			}
			if (elem->mode)
				printf("..%06o", elem->mode);
		}
		printf("%s\n", c_reset);
	}

	if (!show_file_header)
		return;

	if (added)
		dump_quoted_path("--- ", "", "/dev/null",
				 line_prefix, c_meta, c_reset);
	else
		dump_quoted_path("--- ", a_prefix, elem->path,
				 line_prefix, c_meta, c_reset);
	if (deleted)
		dump_quoted_path("+++ ", "", "/dev/null",
				 line_prefix, c_meta, c_reset);
	else
		dump_quoted_path("+++ ", b_prefix, elem->path,
				 line_prefix, c_meta, c_reset);
}

			    int dense, int working_tree_file,
			    struct rev_info *rev)
	struct userdiff_driver *userdiff;
	struct userdiff_driver *textconv = NULL;
	int is_binary;
	const char *line_prefix = diff_line_prefix(opt);
	userdiff = userdiff_find_by_path(elem->path);
	if (!userdiff)
		userdiff = userdiff_find_by_name("default");
	if (DIFF_OPT_TST(opt, ALLOW_TEXTCONV))
		textconv = userdiff_get_textconv(userdiff);
		result = grab_blob(elem->sha1, elem->mode, &result_size,
				   textconv, elem->path);
				result = grab_blob(elem->sha1, elem->mode,
						   &result_size, NULL, NULL);
				result = grab_blob(sha1, elem->mode,
						   &result_size, NULL, NULL);
		} else if (textconv) {
			struct diff_filespec *df = alloc_filespec(elem->path);
			fill_filespec(df, null_sha1, 0, st.st_mode);
			result_size = fill_textconv(textconv, df, &result);
			free_filespec(df);
	for (i = 0; i < num_parent; i++) {
		if (elem->parent[i].mode != elem->mode) {
			mode_differs = 1;
			break;
		}
	}

	if (textconv)
		is_binary = 0;
	else if (userdiff->binary != -1)
		is_binary = userdiff->binary;
	else {
		is_binary = buffer_is_binary(result, result_size);
		for (i = 0; !is_binary && i < num_parent; i++) {
			char *buf;
			unsigned long size;
			buf = grab_blob(elem->parent[i].sha1,
					elem->parent[i].mode,
					&size, NULL, NULL);
			if (buffer_is_binary(buf, size))
				is_binary = 1;
			free(buf);
		}
	}
	if (is_binary) {
		show_combined_header(elem, num_parent, dense, rev,
				     line_prefix, mode_differs, 0);
		printf("Binary files differ\n");
		free(result);
		return;
	}

				     cnt, i, num_parent, result_deleted,
				     textconv, elem->path, opt->xdl_opts);
		show_combined_header(elem, num_parent, dense, rev,
				     line_prefix, mode_differs, 1);
		dump_sline(sline, line_prefix, cnt, num_parent,
			   opt->use_color, result_deleted);
		if (sline[lno].lost) {
			struct lline *ll = sline[lno].lost;
	int line_termination, inter_name_termination, i;
	const char *line_prefix = diff_line_prefix(opt);

		printf("%s", line_prefix);

		/* As many colons as there are parents */
		for (i = 0; i < num_parent; i++)
			putchar(':');
		for (i = 0; i < num_parent; i++)
			printf("%06o ", p->parent[i].mode);
		printf("%06o", p->mode);
/*
 * The result (p->elem) is from the working tree and their
 * parents are typically from multiple stages during a merge
 * (i.e. diff-files) or the state in HEAD and in the index
 * (i.e. diff-index).
 */

		show_patch_diff(p, num_parent, dense, 1, rev);
}

static void free_combined_pair(struct diff_filepair *pair)
{
	free(pair->two);
	free(pair);
}

/*
 * A combine_diff_path expresses N parents on the LHS against 1 merge
 * result. Synthesize a diff_filepair that has N entries on the "one"
 * side and 1 entry on the "two" side.
 *
 * In the future, we might want to add more data to combine_diff_path
 * so that we can fill fields we are ignoring (most notably, size) here,
 * but currently nobody uses it, so this should suffice for now.
 */
static struct diff_filepair *combined_pair(struct combine_diff_path *p,
					   int num_parent)
{
	int i;
	struct diff_filepair *pair;
	struct diff_filespec *pool;

	pair = xmalloc(sizeof(*pair));
	pool = xcalloc(num_parent + 1, sizeof(struct diff_filespec));
	pair->one = pool + 1;
	pair->two = pool;

	for (i = 0; i < num_parent; i++) {
		pair->one[i].path = p->path;
		pair->one[i].mode = p->parent[i].mode;
		hashcpy(pair->one[i].sha1, p->parent[i].sha1);
		pair->one[i].sha1_valid = !is_null_sha1(p->parent[i].sha1);
		pair->one[i].has_more_entries = 1;
	}
	pair->one[num_parent - 1].has_more_entries = 0;

	pair->two->path = p->path;
	pair->two->mode = p->mode;
	hashcpy(pair->two->sha1, p->sha1);
	pair->two->sha1_valid = !is_null_sha1(p->sha1);
	return pair;
}

static void handle_combined_callback(struct diff_options *opt,
				     struct combine_diff_path *paths,
				     int num_parent,
				     int num_paths)
{
	struct combine_diff_path *p;
	struct diff_queue_struct q;
	int i;

	q.queue = xcalloc(num_paths, sizeof(struct diff_filepair *));
	q.alloc = num_paths;
	q.nr = num_paths;
	for (i = 0, p = paths; p; p = p->next) {
		if (!p->len)
			continue;
		q.queue[i++] = combined_pair(p, num_parent);
	}
	opt->format_callback(&q, opt, opt->format_callback_data);
	for (i = 0; i < num_paths; i++)
		free_combined_pair(q.queue[i]);
	free(q.queue);
			const struct sha1_array *parents,
	int i, num_paths, needsep, show_log_first, num_parent = parents->nr;
	copy_pathspec(&diffopts.pathspec, &opt->pathspec);
		diff_tree_sha1(parents->sha1[i], sha1, "", &diffopts);

				printf("%s%c", diff_line_prefix(opt),
				       opt->line_termination);
		else if (opt->output_format & DIFF_FORMAT_CALLBACK)
			handle_combined_callback(opt, paths, num_parent, num_paths);

				printf("%s%c", diff_line_prefix(opt),
				       opt->line_termination);
							0, rev);

	free_pathspec(&diffopts.pathspec);
void diff_tree_combined_merge(const struct commit *commit, int dense,
			      struct rev_info *rev)
	struct commit_list *parent = get_saved_parents(rev, commit);
	struct sha1_array parents = SHA1_ARRAY_INIT;

	while (parent) {
		sha1_array_append(&parents, parent->item->object.sha1);
		parent = parent->next;
	}
	diff_tree_combined(commit->object.sha1, &parents, dense, rev);
	sha1_array_clear(&parents);