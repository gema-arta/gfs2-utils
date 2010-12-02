#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <libintl.h>
#define _(String) gettext(String)
#include <linux_endian.h>
#include <linux/gfs2_ondisk.h>
#include "tunegfs2.h"

static int str_to_hexchar(const char *estring)
{
	int ch = 0;

	if (isdigit(*estring))
		ch = (*estring - '0') * 0x10;
	else if (*estring >= 'a' && *estring <= 'f')
		ch = (*estring - 'a' + 0x0a) * 0x10;
	else if (*estring >= 'A' && *estring <= 'F')
		ch = (*estring - 'A' + 0x0a) * 0x10;

	estring++;
	if (isdigit(*estring))
		ch += (*estring - '0');
	else if (*estring >= 'a' && *estring <= 'f')
		ch += (*estring - 'a' + 0x0a);
	else if (*estring >= 'A' && *estring <= 'F')
		ch += (*estring - 'A' + 0x0a);
	return ch;
}



static const char *uuid2str(const unsigned char *uuid)
{
	static char str[64];
	char *ch;
	int i;

	memset(str, 0, 64);
	ch = str;
	for (i = 0; i < 16; i++) {
		sprintf(ch, "%02x", uuid[i]);
		ch += 2;
		if ((i == 3) || (i == 5) || (i == 7) || (i == 9)) {
			*ch = '-';
			ch++;
		}
	}
	return str;
}

static int str2uuid(const char *newval, char *uuid)
{
	char *cp;
	int i;

	if (strlen(newval) != 36) {
		fprintf(stderr, _("Invalid UUID specified.\n"));
		return -EINVAL;
	}

	cp = uuid;
	for (i = 0; i < 36; i++) {
		if ((i == 8) || (i == 13) ||
				(i == 18) || (i == 23)) {
			if (newval[i] == '-')
				continue;
			fprintf(stderr, _("uuid %s has an invalid format."),
					newval);
			return -EINVAL;
		}
		if (!isxdigit(newval[i])) {
			fprintf(stderr, _("uuid %s has an invalid hex "
						"digit '%c' at offset %d.\n"),
					newval, newval[i], i + 1);
			return -EINVAL;
		}
		*cp = str_to_hexchar(&newval[i++]);
		cp++;
	}
	return 0;
}

int read_super(struct tunegfs2 *tfs)
{
	void *block;
	int n;
       	tfs->sb_start = GFS2_SB_ADDR << GFS2_BASIC_BLOCK_SHIFT;
	block = malloc(sizeof(char) * GFS2_DEFAULT_BSIZE);
	n = pread(tfs->fd, block, GFS2_DEFAULT_BSIZE, tfs->sb_start);
	if (n < 0) {
		fprintf(stderr, _("Error reading from device"));
		return errno;
	}
	tfs->sb = block;
	if (be32_to_cpu(tfs->sb->sb_header.mh_magic) != GFS2_MAGIC) {
		fprintf(stderr, _("Not a GFS/GFS2 device\n"));
		return -EINVAL;
	}
	return 0;
}

static int is_gfs2(const struct tunegfs2 *tfs)
{
	return be32_to_cpu(tfs->sb->sb_fs_format) == GFS2_FORMAT_FS;
}

int print_super(const struct tunegfs2 *tfs)
{
	char *fsname = NULL;
	int table_len = 0, fsname_len = 0;

	fsname = strchr(tfs->sb->sb_locktable, ':');
	if (fsname) {
		table_len = fsname - tfs->sb->sb_locktable;
		fsname_len = GFS2_LOCKNAME_LEN - table_len - 1;
		fsname++;
	}

	printf(_("Filesystem volume name: %.*s\n"), fsname_len, fsname);
	if (is_gfs2(tfs))
		printf(_("Filesystem UUID: %s\n"), uuid2str(tfs->sb->sb_uuid));
	printf( _("Filesystem magic number: 0x%X\n"), be32_to_cpu(tfs->sb->sb_header.mh_magic));
	printf(_("Block size: %d\n"), be32_to_cpu(tfs->sb->sb_bsize));
	printf(_("Block shift: %d\n"), be32_to_cpu(tfs->sb->sb_bsize_shift));
	printf(_("Root inode: %llu\n"), (unsigned long long)be64_to_cpu(tfs->sb->sb_root_dir.no_addr));
	if (is_gfs2(tfs))
		printf(_("Master inode: %llu\n"), (unsigned long long)be64_to_cpu(tfs->sb->sb_master_dir.no_addr));
	printf(_("Lock Protocol: %.*s\n"), GFS2_LOCKNAME_LEN,
		tfs->sb->sb_lockproto);
	printf(_("Lock table: %.*s\n"), table_len, tfs->sb->sb_locktable);

	return 0;
}

int write_super(const struct tunegfs2 *tfs)
{
	int n;
	n = pwrite(tfs->fd, tfs->sb, GFS2_DEFAULT_BSIZE, tfs->sb_start);
	if (n<0) {
		fprintf(stderr, _("Unable to write super block\n"));
		return -errno;
	}
	return 0;
}

int change_label(struct tunegfs2 *tfs, const char *fsname)
{
	char *sb_fsname = NULL;
	int l = strlen(fsname), table_len = 0, fsname_len = 0;

	sb_fsname = strchr(tfs->sb->sb_locktable, ':');
	if (sb_fsname) {
		table_len = sb_fsname - tfs->sb->sb_locktable;
		fsname_len = GFS2_LOCKNAME_LEN - table_len - 1;
		sb_fsname++;
	}
	if (fsname_len < l) {
		fprintf(stderr, _("Label too long\n"));
		return -E2BIG;
	}
	memset(sb_fsname, '\0', fsname_len);
	memcpy(sb_fsname, fsname, l);
	return 0;
}

int change_uuid(struct tunegfs2 *tfs, const char *str)
{
	char uuid[16];
	int status = 0;
	if (be32_to_cpu(tfs->sb->sb_header.mh_magic) != GFS2_MAGIC) {
		fprintf(stderr, _("UUID can be changed for a GFS2"));
		fprintf(stderr, _(" device only\n"));
		return -EINVAL;
	}
	status = str2uuid(str, uuid);
	if (!status)
		memcpy(tfs->sb->sb_uuid, uuid, 16);
	return status;
}


int change_lockproto(struct tunegfs2 *tfs, const char *lockproto)
{
	int l = strlen(lockproto);
	if (strncmp(lockproto, "lock_dlm", 8) 
			&& strncmp(lockproto, "lock_nolock", 11)) {
		fprintf(stderr, _("Incorrect lock protocol specified\n"));
		return -EINVAL;
	}
	memset(tfs->sb->sb_lockproto, '\0', GFS2_LOCKNAME_LEN);
	strncpy(tfs->sb->sb_lockproto, lockproto, l);
	return 0;
}

int change_locktable(struct tunegfs2 *tfs, const char *locktable)
{
	char *sb_fsname = NULL;
	char t_fsname[GFS2_LOCKNAME_LEN];
	int l = strlen(locktable), table_len = 0, fsname_len = 0;

	sb_fsname = strchr(tfs->sb->sb_locktable, ':');
	if (sb_fsname) {
		table_len = sb_fsname - tfs->sb->sb_locktable;
		fsname_len = GFS2_LOCKNAME_LEN - table_len - 1;
		sb_fsname++;
	}
	/* Gotta check if the existing fsname will allow us to fit in
	 * the new locktable name */
	fsname_len = strlen(sb_fsname);
	if (fsname_len > GFS2_LOCKNAME_LEN - table_len - 1)
		fsname_len = GFS2_LOCKNAME_LEN - table_len - 1;

	if (l > GFS2_LOCKNAME_LEN - fsname_len - 1) {
		fprintf(stderr, _("Lock table name too big\n"));
		return -E2BIG;
	}
	memset(t_fsname, '\0', GFS2_LOCKNAME_LEN);
	strncpy(t_fsname, sb_fsname, fsname_len);
	memset(tfs->sb->sb_locktable, '\0', GFS2_LOCKNAME_LEN);
	sprintf(tfs->sb->sb_locktable, "%s:%s", locktable, t_fsname);
	return 0;
}

