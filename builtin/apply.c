#include "lockfile.h"
static int unsafe_paths;
	N_("git apply [<options>] [<patch>...]"),
	struct object_id threeway_stage[3];
	/* skip leading whitespaces, if both begin with whitespace */
	if (s1 <= last1 && s2 <= last2 && isspace(*s1) && isspace(*s2)) {
		while (isspace(*s1) && (s1 <= last1))
			s1++;
		while (isspace(*s2) && (s2 <= last2))
			s2++;
	}
	return skip_prefix(str, "/dev/null", &str) && isspace(*str);
		return squash_slash(xstrdup_or_null(def));
		return squash_slash(xstrdup_or_null(def));
			patch->new_name = xstrdup_or_null(name);
	patch->old_name = xstrdup_or_null(patch->def_name);
	patch->new_name = xstrdup_or_null(patch->def_name);
	eol = strchrnul(line, '\n');
		char *s = xstrfmt("%s%s", root, patch->def_name);
			if (!apply_in_reverse &&
			    ws_error_action == correct_ws_error)
				check_whitespace(line, len, patch->ws_rule);
	if (!deleted && !added)
		return -1;

static void prefix_one(char **name)
{
	char *old_name = *name;
	if (!old_name)
		return;
	*name = xstrdup(prefix_filename(prefix, prefix_length, *name));
	free(old_name);
}

static void prefix_patch(struct patch *p)
{
	if (!prefix || p->is_toplevel_relative)
		return;
	prefix_one(&p->new_name);
	prefix_one(&p->old_name);
}

/*
 * include/exclude
 */

static struct string_list limit_by_name;
static int has_include;
static void add_name_limit(const char *name, int exclude)
{
	struct string_list_item *it;

	it = string_list_append(&limit_by_name, name);
	it->util = exclude ? NULL : (void *) 1;
}

static int use_patch(struct patch *p)
{
	const char *pathname = p->new_name ? p->new_name : p->old_name;
	int i;

	/* Paths outside are not touched regardless of "--include" */
	if (0 < prefix_length) {
		int pathlen = strlen(pathname);
		if (pathlen <= prefix_length ||
		    memcmp(prefix, pathname, prefix_length))
			return 0;
	}

	/* See if it matches any of exclude/include rule */
	for (i = 0; i < limit_by_name.nr; i++) {
		struct string_list_item *it = &limit_by_name.items[i];
		if (!wildmatch(it->string, pathname, 0, NULL))
			return (it->util != NULL);
	}

	/*
	 * If we had any include, a path that does not match any rule is
	 * not used.  Otherwise, we saw bunch of exclude rules (or none)
	 * and such a path is used.
	 */
	return !has_include;
}


	prefix_patch(patch);

	if (!use_patch(patch))
		patch->ws_rule = 0;
	else
		patch->ws_rule = whitespace_rule(patch->new_name
						 ? patch->new_name
						 : patch->old_name);
			static const char *binhdr[] = {
				"Binary files ",
				"Files ",
				NULL,
			};
			int i;
	if (postlen
	    ? postlen < new - postimage->buf
	    : postimage->len < new - postimage->buf)
		die("BUG: caller miscounted postlen: asked %d, orig = %d, used = %d",
		    (int)postlen, (int) postimage->len, (int)(new - postimage->buf));

	 * it might with whitespace fuzz. We weren't asked to
	 * While checking the preimage against the target, whitespace
	 * errors in both fixed, we count how large the corresponding
	 * postimage needs to be.  The postimage prepared by
	 * apply_one_fragment() has whitespace errors fixed on added
	 * lines already, but the common lines were propagated as-is,
	 * which may become longer when their whitespace errors are
	 * fixed.
	 */

	/* First count added lines in postimage */
	postlen = 0;
	for (i = 0; i < postimage->nr; i++) {
		if (!(postimage->line[i].flag & LINE_COMMON))
			postlen += postimage->line[i].len;
	}

	/*

		/* Add the length if this is common with the postimage */
		if (preimage->line[i].flag & LINE_COMMON)
			postlen += tgtfix.len;
		REALLOC_ARRAY(img->line, nr);
			applied_pos = -1;
			goto out;
out:
		img->buf = xmemdupz(fragment->patch, img->len);
static int checkout_target(struct index_state *istate,
			   struct cache_entry *ce, struct stat *st)
	costate.istate = istate;
	if (cached || check_index) {
		} else if (has_symlink_leading_path(name, strlen(name))) {
			return error(_("reading from '%s' beyond a symbolic link"), name);
		if (checkout_target(&the_index, ce, &st))
			oidclr(&patch->threeway_stage[0]);
			hashcpy(patch->threeway_stage[0].hash, pre_sha1);
		hashcpy(patch->threeway_stage[1].hash, our_sha1);
		hashcpy(patch->threeway_stage[2].hash, post_sha1);
			if (checkout_target(&the_index, *ce, st))
/*
 * We need to keep track of how symlinks in the preimage are
 * manipulated by the patches.  A patch to add a/b/c where a/b
 * is a symlink should not be allowed to affect the directory
 * the symlink points at, but if the same patch removes a/b,
 * it is perfectly fine, as the patch removes a/b to make room
 * to create a directory a/b so that a/b/c can be created.
 */
static struct string_list symlink_changes;
#define SYMLINK_GOES_AWAY 01
#define SYMLINK_IN_RESULT 02

static uintptr_t register_symlink_changes(const char *path, uintptr_t what)
{
	struct string_list_item *ent;

	ent = string_list_lookup(&symlink_changes, path);
	if (!ent) {
		ent = string_list_insert(&symlink_changes, path);
		ent->util = (void *)0;
	}
	ent->util = (void *)(what | ((uintptr_t)ent->util));
	return (uintptr_t)ent->util;
}

static uintptr_t check_symlink_changes(const char *path)
{
	struct string_list_item *ent;

	ent = string_list_lookup(&symlink_changes, path);
	if (!ent)
		return 0;
	return (uintptr_t)ent->util;
}

static void prepare_symlink_changes(struct patch *patch)
{
	for ( ; patch; patch = patch->next) {
		if ((patch->old_name && S_ISLNK(patch->old_mode)) &&
		    (patch->is_rename || patch->is_delete))
			/* the symlink at patch->old_name is removed */
			register_symlink_changes(patch->old_name, SYMLINK_GOES_AWAY);

		if (patch->new_name && S_ISLNK(patch->new_mode))
			/* the symlink at patch->new_name is created or remains */
			register_symlink_changes(patch->new_name, SYMLINK_IN_RESULT);
	}
}

static int path_is_beyond_symlink_1(struct strbuf *name)
{
	do {
		unsigned int change;

		while (--name->len && name->buf[name->len] != '/')
			; /* scan backwards */
		if (!name->len)
			break;
		name->buf[name->len] = '\0';
		change = check_symlink_changes(name->buf);
		if (change & SYMLINK_IN_RESULT)
			return 1;
		if (change & SYMLINK_GOES_AWAY)
			/*
			 * This cannot be "return 0", because we may
			 * see a new one created at a higher level.
			 */
			continue;

		/* otherwise, check the preimage */
		if (check_index) {
			struct cache_entry *ce;

			ce = cache_file_exists(name->buf, name->len, ignore_case);
			if (ce && S_ISLNK(ce->ce_mode))
				return 1;
		} else {
			struct stat st;
			if (!lstat(name->buf, &st) && S_ISLNK(st.st_mode))
				return 1;
		}
	} while (1);
	return 0;
}

static int path_is_beyond_symlink(const char *name_)
{
	int ret;
	struct strbuf name = STRBUF_INIT;

	assert(*name_ != '\0');
	strbuf_addstr(&name, name_);
	ret = path_is_beyond_symlink_1(&name);
	strbuf_release(&name);

	return ret;
}

static void die_on_unsafe_path(struct patch *patch)
{
	const char *old_name = NULL;
	const char *new_name = NULL;
	if (patch->is_delete)
		old_name = patch->old_name;
	else if (!patch->is_new && !patch->is_copy)
		old_name = patch->old_name;
	if (!patch->is_delete)
		new_name = patch->new_name;

	if (old_name && !verify_path(old_name))
		die(_("invalid path '%s'"), old_name);
	if (new_name && !verify_path(new_name))
		die(_("invalid path '%s'"), new_name);
}

	if (!unsafe_paths)
		die_on_unsafe_path(patch);

	/*
	 * An attempt to read from or delete a path that is beyond a
	 * symbolic link will be prevented by load_patch_target() that
	 * is called at the beginning of apply_data() so we do not
	 * have to worry about a patch marked with "is_delete" bit
	 * here.  We however need to make sure that the patch result
	 * is not deposited to a path that is beyond a symbolic link
	 * here.
	 */
	if (!patch->is_delete && path_is_beyond_symlink(patch->new_name))
		return error(_("affected file '%s' is beyond a symbolic link"),
			     patch->new_name);

	prepare_symlink_changes(patch);
	static struct lock_file lock;
				die("sha1 information is lacking or useless for submodule %s",
	hold_lock_file_for_update(&lock, filename, LOCK_DIE_ON_ERROR);
	if (write_locked_index(&result, &lock, COMMIT_LOCK))
		const char *s;
		if (!skip_prefix(buf, "Subproject commit ", &s) ||
		    get_sha1_hex(s, ce->sha1))
		if (is_null_oid(&patch->threeway_stage[stage - 1]))
		hashcpy(ce->sha1, patch->threeway_stage[stage - 1].hash);
		string_list_sort(&cpath);
static void git_apply_config(void)
	git_config_get_string_const("apply.whitespace", &apply_default_whitespace);
	git_config_get_string_const("apply.ignorewhitespace", &apply_default_ignorewhitespace);
	git_config(git_default_config, NULL);
		OPT_BOOL(0, "unsafe-paths", &unsafe_paths,
			N_("accept a patch that touches outside the working area")),
	git_apply_config();
	if (check_index)
		unsafe_paths = 0;

		if (write_locked_index(&the_index, &lock_file, COMMIT_LOCK))