#include "clusterautoconfig.h"

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdio.h>
#include <libintl.h>
#define _(String) gettext(String)

#include "libgfs2.h"
#include "fs_bits.h"
#include "util.h"

/* Put out a warm, fuzzy message every second so the user     */
/* doesn't think we hung.  (This may take a long time).       */
void warm_fuzzy_stuff(uint64_t block)
{
	static uint64_t one_percent = 0;
	static struct timeval tv;
	static uint32_t seconds = 0;
	
	if (!one_percent)
		one_percent = last_fs_block / 100;
	if (block - last_reported_block >= one_percent) {
		last_reported_block = block;
		gettimeofday(&tv, NULL);
		if (!seconds)
			seconds = tv.tv_sec;
		if (tv.tv_sec - seconds) {
			static uint64_t percent;

			seconds = tv.tv_sec;
			if (last_fs_block) {
				percent = (block * 100) / last_fs_block;
				log_notice( _("\r%" PRIu64 " percent complete.\r"), percent);
			}
		}
	}
}

const char *block_type_string(struct gfs2_block_query *q)
{
	const char *blktyp[] = {"free", "used", "indirect data", "inode",
							"file", "symlink", "block dev", "char dev",
							"fifo", "socket", "dir leaf", "journ data",
							"other meta", "eattribute", "unused",
							"invalid"};
	if (q->block_type < 16)
		return (blktyp[q->block_type]);
	return blktyp[15];
}
