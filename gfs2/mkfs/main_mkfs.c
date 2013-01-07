#include "clusterautoconfig.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <stdarg.h>
#include <mntent.h>
#include <ctype.h>
#include <poll.h>
#include <signal.h>
#include <sys/time.h>
#include <libintl.h>
#include <sys/ioctl.h>

#define _(String) gettext(String)

#include <linux/types.h>
#include "libgfs2.h"
#include "gfs2_mkfs.h"

int discard = 1;

/**
 * This function is for libgfs2's sake.
 */
void print_it(const char *label, const char *fmt, const char *fmt2, ...)
{
	va_list args;

	va_start(args, fmt2);
	printf("%s: ", label);
	vprintf(fmt, args);
	va_end(args);
}

/**
 * print_usage - print out usage information
 * @prog_name: The name of this program
 */

static void
print_usage(const char *prog_name)
{
	int i;
	const char *option, *param, *desc;
	const char *options[] = {
	    /* Translators: This is a usage string printed with --help.
	       <size> and <number> here are  to commandline parameters,
	       e.g. mkfs.gfs2 -b <size> -j <number> /dev/sda */
	    "-b", _("<size>"),   _("File system block size, in bytes"),
	    "-c", _("<size>"),   _("Size of quota change file, in megabytes"),
	    "-D", NULL,          _("Enable debugging code"),
	    "-h", NULL,          _("Display this help, then exit"),
	    "-J", _("<size>"),   _("Size of journals, in megabytes"),
	    "-j", _("<number>"), _("Number of journals"),
	    "-K", NULL,          _("Don't try to discard unused blocks"),
	    "-O", NULL,          _("Don't ask for confirmation"),
	    "-p", _("<name>"),   _("Name of the locking protocol"),
	    "-q", NULL,          _("Don't print anything"),
	    "-r", _("<size>"),   _("Size of resource groups, in megabytes"),
	    "-t", _("<name>"),   _("Name of the lock table"),
	    "-V", NULL,          _("Display program version information, then exit"),
	    NULL, NULL, NULL /* Must be kept at the end */
	};

	printf("%s\n", _("Usage:"));
	printf("%s [%s] <%s> [%s]\n\n", prog_name, _("options"), _("device"), _("size"));
	printf(_("Create a gfs2 file system on a device. If a size, in blocks, is not "
	         "specified, the whole device will be used."));
	printf("\n\n%s\n", _("Options:"));

	for (i = 0; options[i] != NULL; i += 3) {
		option = options[i];
		param = options[i+1];
		desc = options[i+2];
		printf("%3s %-15s %s\n", option, param ? param : "", desc);
	}
}

#ifndef BLKDISCARD
#define BLKDISCARD      _IO(0x12,119)
#endif

static int discard_blocks(struct gfs2_sbd *sdp)
{
        __uint64_t range[2];

	range[0] = 0;
	range[1] = sdp->device.length * sdp->bsize;
	if (sdp->debug)
		/* Translators: "discard" is a request sent to a storage device to
		 * discard a range of blocks. */
		printf(_("Issuing discard request: range: %llu - %llu..."),
		       (unsigned long long)range[0],
		       (unsigned long long)range[1]);
	if (ioctl(sdp->device_fd, BLKDISCARD, &range) < 0) {
		if (sdp->debug)
			printf("%s = %d\n", _("error"), errno);
		return errno;
	}
	if (sdp->debug)
		printf(_("Successful.\n"));
        return 0;
}

/**
 * decode_arguments - decode command line arguments and fill in the struct gfs2_sbd
 * @argc:
 * @argv:
 * @sdp: the decoded command line arguments
 *
 */

static void decode_arguments(int argc, char *argv[], struct gfs2_sbd *sdp)
{
	int cont = TRUE;
	int optchar;

	memset(sdp->device_name, 0, sizeof(sdp->device_name));
	sdp->md.journals = 1;
	sdp->orig_fssize = 0;

	while (cont) {
		optchar = getopt(argc, argv, "-b:c:DhJ:j:KOp:qr:t:VX");

		switch (optchar) {
		case 'b':
			sdp->bsize = atoi(optarg);
			break;

		case 'c':
			sdp->qcsize = atoi(optarg);
			break;

		case 'D':
			sdp->debug = TRUE;
			break;

		case 'h':
			print_usage(argv[0]);
			exit(0);
			break;

		case 'J':
			sdp->jsize = atoi(optarg);
			break;

		case 'j':
			sdp->md.journals = atoi(optarg);
			break;

		case 'K':
			discard = 0;
			break;

		case 'O':
			sdp->override = TRUE;
			break;

		case 'p':
			if (strlen(optarg) >= GFS2_LOCKNAME_LEN)
				die( _("lock protocol name '%s' is too long\n"),
				    optarg);
			strcpy(sdp->lockproto, optarg);
			break;

		case 'q':
			sdp->quiet = TRUE;
			break;

		case 'r':
			sdp->rgsize = atoi(optarg);
			break;

		case 't':
			if (strlen(optarg) >= GFS2_LOCKNAME_LEN)
				die( _("lock table name '%s' is too long\n"), optarg);
			strcpy(sdp->locktable, optarg);
			break;

		case 'V':
			printf("mkfs.gfs2 %s (built %s %s)\n", VERSION,
			       __DATE__, __TIME__);
			printf(REDHAT_COPYRIGHT "\n");
			exit(EXIT_SUCCESS);
			break;

		case 'X':
			sdp->expert = TRUE;
			break;

		case ':':
		case '?':
			fprintf(stderr, _("Please use '-h' for help.\n"));
			exit(EXIT_FAILURE);
			break;

		case EOF:
			cont = FALSE;
			break;

		case 1:
			if (strcmp(optarg, "gfs2") == 0)
				continue;
			if (!sdp->device_name[0])
				strcpy(sdp->device_name, optarg);
			else if (!sdp->orig_fssize &&
				 isdigit(optarg[0]))
				sdp->orig_fssize = atol(optarg);
			else
				die( _("More than one device specified (try -h for help)\n"));
			break;

		default:
			die( _("Invalid option: %c\n"), optchar);
			break;
		};
	}

	if ((sdp->device_name[0] == 0) && (optind < argc))
		strcpy(sdp->device_name, argv[optind++]);

	if (sdp->device_name[0] == '\0')
		die( _("No device specified. Please use '-h' for help\n"));

	if (optind < argc)
		sdp->orig_fssize = atol(argv[optind++]);

	if (optind < argc)
		die( _("Unrecognized argument: %s\n"), argv[optind]);

	if (sdp->debug) {
		printf( _("Command Line Arguments:\n"));
		if (sdp->bsize != -1)
			printf("  bsize = %u\n", sdp->bsize);
		printf("  qcsize = %u\n", sdp->qcsize);
		printf("  jsize = %u\n", sdp->jsize);
		printf("  journals = %u\n", sdp->md.journals);
		printf("  override = %d\n", sdp->override);
		printf("  proto = %s\n", sdp->lockproto);
		printf("  quiet = %d\n", sdp->quiet);
		if (sdp->rgsize == (unsigned int)-1)
			printf("  rgsize = %s\n", _("Optimize for best performance"));
		else
			printf("  rgsize = %u\n", sdp->rgsize);
		printf("  table = %s\n", sdp->locktable);
		printf("  device = %s\n", sdp->device_name);
		if (sdp->orig_fssize)
			printf("  block-count = %llu\n",
			       (unsigned long long)sdp->orig_fssize);
	}
}

/**
 * test_locking - Make sure the GFS2 is set up to use the right lock protocol
 * @lockproto: the lock protocol to mount
 * @locktable: the locktable name
 *
 */

static void test_locking(char *lockproto, char *locktable)
{
	char *c;
	/* Translators: A lock table is a string identifying a gfs2 file system
	 * in a cluster, e.g. cluster_name:fs_name */
	const char *errprefix = _("Invalid lock table:");

	if (strcmp(lockproto, "lock_nolock") == 0) {
		/*  Nolock is always ok.  */
	} else if (strcmp(lockproto, "lock_gulm") == 0 ||
		   strcmp(lockproto, "lock_dlm") == 0) {
		if (locktable == NULL || *locktable == '\0') {
			fprintf(stderr, _("No lock table specified.\n"));
			exit(-1);
		}
		for (c = locktable; *c; c++) {
			if (!isalnum(*c) && (*c != '-') && (*c != '_') && (*c != ':'))
				die("%s %s '%c'\n", errprefix, _("invalid character"), *c);
		}

		c = strstr(locktable, ":");
		if (!c)
			die("%s %s\n", errprefix, _("missing colon"));

		if (c == locktable)
			die("%s %s\n", errprefix, _("cluster name is missing"));
		if (c - locktable > 16)
			die("%s %s\n", errprefix, _("cluster name is too long"));

		c++;
		if (strstr(c, ":"))
			die("%s %s\n", errprefix, _("contains more than one colon"));
		if (!strlen(c))
			die("%s %s\n", errprefix, _("file system name is missing"));
		if (strlen(c) > 16)
			die("%s %s\n", errprefix, _("file system name too long"));
	} else {
		die( _("Invalid lock protocol: %s\n"), lockproto);
	}
}

/**
 * are_you_sure - protect lusers from themselves
 * @sdp: the command line
 *
 */

static void are_you_sure(void)
{
	char *line = NULL;
	size_t len = 0;
	int ret = -1;
	int res = 0;

	do{
		/* Translators: We use rpmatch(3) to match the answers to y/n
		   questions in the user's own language, so the [y/n] here must also be
		   translated to match one of the letters in the pattern printed by
		   `locale -k yesexpr` and one of the letters in the pattern printed by
		   `locale -k noexpr` */
		printf( _("Are you sure you want to proceed? [y/n]"));
		ret = getline(&line, &len, stdin);
		res = rpmatch(line);
		
		if (res > 0){
			free(line);
			return;
		}
		if (!res){
			printf("\n");
			die( _("Aborted.\n"));
		}
		
	}while(ret >= 0);

	if(line)
		free(line);
}

static void verify_bsize(struct gfs2_sbd *sdp)
{
	unsigned int x;

	/* Block sizes must be a power of two from 512 to 65536 */

	for (x = 512; x; x <<= 1)
		if (x == sdp->bsize)
			break;

	if (!x || sdp->bsize > getpagesize())
		die( _("Block size must be a power of two between 512 and %d\n"),
		       getpagesize());

	if (sdp->bsize < sdp->dinfo.logical_block_size) {
		die( _("Error: Block size %d is less than minimum logical "
		       "block size (%d).\n"), sdp->bsize,
		     sdp->dinfo.logical_block_size);
	}

	if (sdp->bsize < sdp->dinfo.physical_block_size) {
		printf( _("WARNING: Block size %d is inefficient because it "
			  "is less than the physical block size (%d).\n"),
			  sdp->bsize, sdp->dinfo.physical_block_size);
		if (sdp->override)
			return;

		are_you_sure();
	}
}

static void verify_arguments(struct gfs2_sbd *sdp)
{
	if (!sdp->expert)
		test_locking(sdp->lockproto, sdp->locktable);
	if (sdp->expert) {
		if (GFS2_EXP_MIN_RGSIZE > sdp->rgsize || sdp->rgsize > GFS2_MAX_RGSIZE)
			/* Translators: gfs2 file systems are split into equal sized chunks called
			   resource groups. We're checking that the user gave a valid size for them. */
			die( _("bad resource group size\n"));
	} else {
		if (GFS2_MIN_RGSIZE > sdp->rgsize || sdp->rgsize > GFS2_MAX_RGSIZE)
			die( _("bad resource group size\n"));
	}

	if (!sdp->md.journals)
		die( _("no journals specified\n"));

	if (sdp->jsize < 8 || sdp->jsize > 1024)
		die( _("bad journal size\n"));

	if (!sdp->qcsize || sdp->qcsize > 64)
		die( _("bad quota change size\n"));
}

static int get_file_output(int fd, char *buffer, size_t buflen)
{
	struct pollfd pf = { .fd = fd, .events = POLLIN|POLLRDHUP };
	int flags;
	int pos = 0;
	int rv;

	flags = fcntl(fd, F_GETFL, 0);
	if (flags < 0)
		return flags;

	flags |= O_NONBLOCK;
	rv = fcntl(fd, F_SETFL, flags);
	if (rv < 0)
		return rv;

	while (1) {
		rv = poll(&pf, 1, 10 * 1000);
		if (rv == 0)
			break;
		if (rv < 0)
			return rv;
		if (pf.revents & POLLIN) {
			rv = read(fd, buffer + pos,
				   buflen - pos);
			if (rv < 0) {
				if (errno == EAGAIN)
					continue;
				return rv;
			}
			if (rv == 0)
				break;
			pos += rv;
			if (pos >= buflen)
				return -1;
			buffer[pos] = 0;
			continue;
		}
		if (pf.revents & (POLLRDHUP | POLLHUP | POLLERR))
			break;
	}
	return 0;
}

static void check_dev_content(const char *devname)
{
	struct sigaction sa;
	char content[1024] = { 0, };
	char * args[] = {
		(char *)"/usr/bin/file",
		(char *)"-bsL",
		(char *)devname,
		NULL };
	int p[2] = {-1, -1};
	int ret;
	int pid;

	ret = sigaction(SIGCHLD, NULL, &sa);
	if  (ret)
		return;
	sa.sa_handler = SIG_IGN;
	sa.sa_flags |= (SA_NOCLDSTOP | SA_NOCLDWAIT);
	ret = sigaction(SIGCHLD, &sa, NULL);
	if (ret)
		goto fail;

	ret = pipe(p);
	if (ret)
		goto fail;

	pid = fork();

	if (pid < 0) {
		close(p[1]);
		goto fail;
	}

	if (pid) {
		close(p[1]);
		ret = get_file_output(p[0], content, sizeof(content));
		if (ret) {
fail:
			printf( _("Content of file or device unknown (do you have GNU fileutils installed?)\n"));
		} else {
			if (*content == 0)
				goto fail;
			printf( _("It appears to contain: %s"), content);
		}
		if (p[0] >= 0)
			close(p[0]);
		return;
	}

	close(p[0]);
	dup2(p[1], STDOUT_FILENO);
	close(STDIN_FILENO);
	exit(execv(args[0], args));
}


/**
 * print_results - print out summary information
 * @sdp: the command line
 *
 */

static void
print_results(struct gfs2_sbd *sdp, uint64_t real_device_size,
	      unsigned char uuid[16])
{
	if (sdp->debug)
		printf("\n");
	else if (sdp->quiet)
		return;

	if (sdp->expert)
		printf("%-27s%s\n", _("Expert mode:"), _("on"));

	printf("%-27s%s\n", _("Device:"), sdp->device_name);

	printf("%-27s%u\n", _("Block size:"), sdp->bsize);
	printf("%-27s%.2f %s (%llu %s)\n", _("Device size:"),
	       /* Translators: "GB" here means "gigabytes" */
	       real_device_size / ((float)(1 << 30)), _("GB"),
	       (unsigned long long)real_device_size / sdp->bsize, _("blocks"));
	printf("%-27s%.2f %s (%llu %s)\n", _("Filesystem size:"),
	       sdp->fssize / ((float)(1 << 30)) * sdp->bsize, _("GB"),
	       (unsigned long long)sdp->fssize, _("blocks"));
	printf("%-27s%u\n", _("Journals:"), sdp->md.journals);
	printf("%-27s%llu\n", _("Resource groups:"), (unsigned long long)sdp->rgrps);
	printf("%-27s\"%s\"\n", _("Locking protocol:"), sdp->lockproto);
	printf("%-27s\"%s\"\n", _("Lock table:"), sdp->locktable);

	if (sdp->debug) {
		printf("\n%-27s%u\n", _("Writes:"), sdp->writes);
	}
	/* Translators: "UUID" = universally unique identifier. */
	printf("%-27s%s\n\n", _("UUID:"), str_uuid(uuid));
}


/**
 * If path is a symlink, return 1 with *abspath pointing to the absolute path,
 * otherwise return 0. Exit on errors. The caller must free the memory pointed
 * to by *abspath.
 */
static int is_symlink(char *path, char **abspath)
{
	struct stat lnkstat;

	if (lstat(path, &lnkstat) == -1) {
		perror(_("Failed to lstat the device"));
		exit(EXIT_FAILURE);
	}
	if (!S_ISLNK(lnkstat.st_mode)) {
		return 0;
	}
	*abspath = canonicalize_file_name(path);
	if (*abspath == NULL) {
		perror(_("Could not find the absolute path of the device"));
		exit(EXIT_FAILURE);
	}
	/* Translators: Example: "/dev/vg/lv is a symbolic link to /dev/dm-2" */
	printf( _("%s is a symbolic link to %s\n"), path, *abspath);
	return 1;
}

/**
 * main_mkfs - do everything
 * @argc:
 * @argv:
 *
 */
void main_mkfs(int argc, char *argv[])
{
	struct gfs2_sbd sbd, *sdp = &sbd;
	int error;
	int rgsize_specified = 0;
	unsigned char uuid[16];
	char *absname = NULL;
	char *fdpath = NULL;
	int islnk = 0;

	memset(sdp, 0, sizeof(struct gfs2_sbd));
	sdp->bsize = -1;
	sdp->jsize = GFS2_DEFAULT_JSIZE;
	sdp->rgsize = -1;
	sdp->qcsize = GFS2_DEFAULT_QCSIZE;
	strcpy(sdp->lockproto, GFS2_DEFAULT_LOCKPROTO);
	sdp->time = time(NULL);
	sdp->rgtree.osi_node = NULL;

	decode_arguments(argc, argv, sdp);
	if (sdp->rgsize == -1)                 /* if rg size not specified */
		sdp->rgsize = GFS2_DEFAULT_RGSIZE; /* default it for now */
	else
		rgsize_specified = TRUE;

	verify_arguments(sdp);

	sdp->device_fd = open(sdp->device_name, O_RDWR | O_CLOEXEC);
	if (sdp->device_fd < 0){
		perror(sdp->device_name);
		exit(EXIT_FAILURE);
	}

	if (lgfs2_get_dev_info(sdp->device_fd, &sdp->dinfo) < 0) {
		perror(sdp->device_name);
		exit(EXIT_FAILURE);
	}

	if (asprintf(&fdpath, "/proc/%d/fd/%d", getpid(), sdp->device_fd) < 0) {
		perror(_("Failed to build string"));
		exit(EXIT_FAILURE);
	}

	if (!sdp->override) {
		islnk = is_symlink(sdp->device_name, &absname);
		printf(_("This will destroy any data on %s.\n"), islnk ? absname : sdp->device_name);
		free(absname);
		check_dev_content(fdpath);
		are_you_sure();
	}
	free(fdpath);

	if (sdp->bsize == -1) {
		if (S_ISREG(sdp->dinfo.stat.st_mode))
			sdp->bsize = GFS2_DEFAULT_BSIZE;
		/* See if optimal_io_size (the biggest I/O we can submit
		   without incurring a penalty) is a suitable block size. */
		else if (sdp->dinfo.io_optimal_size <= getpagesize() &&
		    sdp->dinfo.io_optimal_size >= sdp->dinfo.io_min_size)
			sdp->bsize = sdp->dinfo.io_optimal_size;
		/* See if physical_block_size (the smallest unit we can write
		   without incurring read-modify-write penalty) is suitable. */
		else if (sdp->dinfo.physical_block_size <= getpagesize() &&
			 sdp->dinfo.physical_block_size >= GFS2_DEFAULT_BSIZE)
			sdp->bsize = sdp->dinfo.physical_block_size;
		else
			sdp->bsize = GFS2_DEFAULT_BSIZE;

		if (sdp->debug)
			printf("\n%s %u\n", _("Using block size:"), sdp->bsize);
	}
	verify_bsize(sdp);

	if (compute_constants(sdp)) {
		perror(_("Failed to compute file system constants"));
		exit(EXIT_FAILURE);
	}

	/* Convert optional block-count to basic blocks */
	if (sdp->orig_fssize) {
		sdp->orig_fssize *= sdp->bsize;
		sdp->orig_fssize >>= GFS2_BASIC_BLOCK_SHIFT;
		if (sdp->orig_fssize > sdp->device.length) {
			fprintf(stderr, "%s:%s\n", argv[0],
			        _("Specified size is bigger than the device."));
			die("%s %.2f %s (%llu %s)\n", _("Device size:"),
			       sdp->dinfo.size / ((float)(1 << 30)), _("GB"),
			       (unsigned long long)sdp->dinfo.size / sdp->bsize, _("blocks"));
		}
		sdp->device.length = sdp->orig_fssize;
	}
	fix_device_geometry(sdp);
	if (!S_ISREG(sdp->dinfo.stat.st_mode) && discard)
		discard_blocks(sdp);

	/* Compute the resource group layouts */

	compute_rgrp_layout(sdp, &sdp->rgtree, rgsize_specified);
	debug_print_rgrps(sdp, &sdp->rgtree);

	/* Generate a random uuid */
	get_random_bytes(uuid, sizeof(uuid));

	/* Build ondisk structures */

	build_rgrps(sdp, TRUE);
	build_root(sdp);
	build_master(sdp);
	build_sb(sdp, uuid);
	error = build_jindex(sdp);
	if (error) {
		/* Translators: "jindex" is the name of a special file */
		perror(_("Error building 'jindex'"));
		exit(EXIT_FAILURE);
	}
	error = build_per_node(sdp);
	if (error) {
		/* Translators: "per-node" is the name of a special directory */
		perror(_("Error building per-node directory"));
		exit(EXIT_FAILURE);
	}
	error = build_inum(sdp);
	if (error) {
		/* Translators: "inum" here is the name of a special file */
		perror(_("Error building 'inum'"));
		exit(EXIT_FAILURE);
	}
	gfs2_lookupi(sdp->master_dir, "inum", 4, &sdp->md.inum);
	error = build_statfs(sdp);
	if (error) {
		/* Translators: "statfs" is the name of a special file */
		perror(_("Error building 'statfs'"));
		exit(EXIT_FAILURE);
	}
	gfs2_lookupi(sdp->master_dir, "statfs", 6, &sdp->md.statfs);
	error = build_rindex(sdp);
	if (error) {
		/* Translators: "rindex" is the name of a special file */
		perror(_("Error building 'rindex'"));
		exit(EXIT_FAILURE);
	}
	error = build_quota(sdp);
	if (error) {
		/* Translators: "quota" is the name of a special file */
		perror(_("Error building 'quota'"));
		exit(EXIT_FAILURE);
	}

	do_init_inum(sdp);
	do_init_statfs(sdp);

	/* Cleanup */

	inode_put(&sdp->md.rooti);
	inode_put(&sdp->master_dir);
	inode_put(&sdp->md.inum);
	inode_put(&sdp->md.statfs);

	gfs2_rgrp_free(&sdp->rgtree);
	error = fsync(sdp->device_fd);

	if (error){
		perror(sdp->device_name);
		exit(EXIT_FAILURE);
	}

	error = close(sdp->device_fd);

	if (error){
		perror(sdp->device_name);
		exit(EXIT_FAILURE);
	}

	print_results(sdp, sdp->dinfo.size, uuid);
}
