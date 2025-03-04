#include "clusterautoconfig.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <curses.h>
#include <term.h>
#include <time.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <dirent.h>

#include "copyright.cf"

#include "hexedit.h"
#include "libgfs2.h"
#include "extended.h"
#include "gfs2hex.h"

static void print_block_details(struct iinfo *ind, int level, int cur_height,
				int pndx, uint64_t file_offset);

static int get_height(void)
{
	int cur_height = 0, i;

	if (gfs2_struct_type != GFS2_METATYPE_DI) {
		for (i = 0; i <= blockhist && i < 5; i++) {
			if (blockstack[(blockhist - i) %
				       BLOCK_STACK_SIZE].gfs2_struct_type ==
			    GFS2_METATYPE_DI)
				break;
			cur_height++;
		}
	}
	return cur_height;
}

static int _do_indirect_extended(char *diebuf, struct iinfo *iinf, int hgt)
{
	unsigned int x, y;
	off_t headoff;
	uint64_t p;
	int i_blocks;

	i_blocks = 0;
	for (x = 0; x < 512; x++) {
		iinf->ii[x].is_dir = 0;
		iinf->ii[x].height = 0;
		iinf->ii[x].block = 0;
		iinf->ii[x].dirents = 0;
		memset(&iinf->ii[x].dirent, 0, sizeof(struct idirent));
	}
	headoff = sbd.gfs1 ? sizeof(struct gfs_indirect) : sizeof(struct gfs2_meta_header);
	for (x = headoff, y = 0; x < sbd.sd_bsize; x += sizeof(uint64_t), y++) {
		p = be64_to_cpu(*(__be64 *)(diebuf + x));
		if (p) {
			iinf->ii[i_blocks].block = p;
			iinf->ii[i_blocks].mp.mp_list[hgt] = i_blocks;
			iinf->ii[i_blocks].is_dir = FALSE;
			iinf->ii[i_blocks].ptroff = (x - headoff) / sizeof(uint64_t);
			i_blocks++;
		}
	}
	return i_blocks;
}

int do_indirect_extended(char *diebuf, struct iinfo *iinf)
{
	return _do_indirect_extended(diebuf, iinf, get_height());
}

/* ------------------------------------------------------------------------ */
/* dinode_valid - check if we have a dinode in recent history               */
/* ------------------------------------------------------------------------ */
static int dinode_valid(void)
{
	int i;

	if (gfs2_struct_type == GFS2_METATYPE_DI)
		return 1;
	for (i = 0; i <= blockhist && i < 5; i++) {
		if (blockstack[(blockhist - i) %
			       BLOCK_STACK_SIZE].gfs2_struct_type ==
		    GFS2_METATYPE_DI)
			return 1;
	}
	return 0;
}

static uint64_t metapath_to_lblock(struct metapath *mp, int hgt)
{
	int h;
	uint64_t lblock = 0;
	uint64_t factor[GFS2_MAX_META_HEIGHT];
	uint16_t height = be16_to_cpu(di->di_height);

	if (height < 2)
		return mp->mp_list[0];
	/* figure out multiplication factors for each height */
	memset(&factor, 0, sizeof(factor));
	factor[height - 1] = 1ull;
	for (h = height - 2; h >= 0; h--)
		factor[h] = factor[h + 1] * sbd.sd_inptrs;
	for (h = 0; h <= hgt; h++)
		lblock += (mp->mp_list[h] * factor[h]);
	return lblock;
}

static int display_indirect(struct iinfo *ind, int indblocks, int level,
			    uint64_t startoff)
{
	int start_line;
	int cur_height = -1, pndx;

	last_entry_onscreen[dmode] = 0;
	if (!has_indirect_blocks())
		return -1;
	if (!level) {
		if (gfs2_struct_type == GFS2_METATYPE_DI) {
			if (S_ISDIR(be32_to_cpu(di->di_mode)))
				print_gfs2("This directory contains %d indirect blocks",
					   indblocks);
			else
				print_gfs2("This inode contains %d indirect blocks",
					   indblocks);
		} else
			print_gfs2("This indirect block contains %d indirect blocks",
				   indblocks);
	}
	if (dinode_valid() && !S_ISDIR(be32_to_cpu(di->di_mode))) {
		/* See if we are on an inode or have one in history. */
		if (level)
			cur_height = level;
		else {
			cur_height = get_height();
			print_gfs2("  (at height %d of %d)",
			           cur_height, be16_to_cpu(di->di_height));
		}
	}
	eol(0);
	if (!level && indblocks) {
		print_gfs2("Indirect blocks:");
		eol(0);
	}
	start_line = line;
	for (pndx = start_row[dmode];
		 (!termlines || pndx < termlines - start_line - 1
		  + start_row[dmode]) && pndx < indblocks;
		 pndx++) {
		uint64_t file_offset;

		if (pndx && ind->ii[pndx].block == ind->ii[pndx - 1].block)
			continue;
		print_entry_ndx = pndx;
		if (termlines) {
			if (edit_row[dmode] >= 0 &&
			    line - start_line ==
			    edit_row[dmode] - start_row[dmode])
				COLORS_HIGHLIGHT;
			move(line, 1);
		}
		if (!termlines) {
			int h;

			for (h = 0; h < level; h++)
				print_gfs2("   ");
		}
		print_gfs2("%d: 0x%"PRIx64" => ", pndx, ind->ii[pndx].ptroff);
		print_gfs2("0x%"PRIx64" / %"PRId64, ind->ii[pndx].block,
			   ind->ii[pndx].block);
		if (termlines) {
			if (edit_row[dmode] >= 0 &&
			    line - start_line ==
			    edit_row[dmode] - start_row[dmode]) { 
				sprintf(estring, "%llx",
					(unsigned long long)ind->ii[print_entry_ndx].block);
				strcpy(edit_fmt, "%llx");
				edit_size[dmode] = strlen(estring);
				COLORS_NORMAL;
			}
		}
		if (dinode_valid() && !S_ISDIR(be32_to_cpu(di->di_mode))) {
			float human_off;
			char h;

			file_offset = metapath_to_lblock(&ind->ii[pndx].mp,
							 cur_height) *
				sbd.sd_bsize;
			print_gfs2("     ");
			h = 'K';
			human_off = (file_offset / 1024.0);
			if (human_off > 1024.0) { h = 'M'; human_off /= 1024.0; }
			if (human_off > 1024.0) { h = 'G'; human_off /= 1024.0; }
			if (human_off > 1024.0) { h = 'T'; human_off /= 1024.0; }
			if (human_off > 1024.0) { h = 'P'; human_off /= 1024.0; }
			if (human_off > 1024.0) { h = 'E'; human_off /= 1024.0; }
			print_gfs2("(data offset 0x%"PRIx64" / %"PRId64" / %6.2f%c)",
				   file_offset, file_offset, human_off, h);
			print_gfs2("   ");
		}
		else
			file_offset = 0;
		if (dinode_valid() && !termlines &&
		    ((level + 1 < be16_to_cpu(di->di_height)) ||
		     (S_ISDIR(be32_to_cpu(di->di_mode)) && level <= be16_to_cpu(di->di_height)))) {
			print_block_details(ind, level, cur_height, pndx,
					    file_offset);
		}
		print_entry_ndx = pndx; /* restore after recursion */
		eol(0);
	} /* for each display row */
	if (line >= 7) /* 7 because it was bumped at the end */
		last_entry_onscreen[dmode] = line - 7;
	eol(0);
	end_row[dmode] = indblocks;
	if (end_row[dmode] < last_entry_onscreen[dmode])
		end_row[dmode] = last_entry_onscreen[dmode];
	lines_per_row[dmode] = 1;
	return 0;
}

static void print_inode_type(uint16_t de_type)
{
	if (sbd.gfs1) {
		switch(de_type) {
		case GFS_FILE_NON:
			print_gfs2("Unknown");
			break;
		case GFS_FILE_REG:
			print_gfs2("File   ");
			break;
		case GFS_FILE_DIR:
			print_gfs2("Dir    ");
			break;
		case GFS_FILE_LNK:
			print_gfs2("Symlink");
			break;
		case GFS_FILE_BLK:
			print_gfs2("BlkDev ");
			break;
		case GFS_FILE_CHR:
			print_gfs2("ChrDev ");
			break;
		case GFS_FILE_FIFO:
			print_gfs2("Fifo   ");
			break;
		case GFS_FILE_SOCK:
			print_gfs2("Socket ");
			break;
		default:
			print_gfs2("%04x   ", de_type);
			break;
		}
		return;
	}
	switch(de_type) {
	case DT_UNKNOWN:
		print_gfs2("Unknown");
		break;
	case DT_REG:
		print_gfs2("File   ");
		break;
	case DT_DIR:
		print_gfs2("Dir    ");
		break;
	case DT_LNK:
		print_gfs2("Symlink");
		break;
	case DT_BLK:
		print_gfs2("BlkDev ");
		break;
	case DT_CHR:
		print_gfs2("ChrDev ");
		break;
	case DT_FIFO:
		print_gfs2("Fifo   ");
		break;
	case DT_SOCK:
		print_gfs2("Socket ");
		break;
	default:
		print_gfs2("%04x   ", de_type);
		break;
	}
}

static int display_leaf(struct iinfo *ind)
{
	struct gfs2_leaf *leaf = &ind->ii[0].lf;
	int start_line, total_dirents = start_row[dmode];
	int d;

	eol(0);
	if (gfs2_struct_type == GFS2_METATYPE_SB)
		print_gfs2("The superblock has 2 directories");
	else
		print_gfs2("Directory block: "
		           "lf_depth:%"PRIu16", lf_entries:%"PRIu16", lf_inode: 0x%"PRIx64", "
		           "lf_dist: %"PRIu32", lf_nsec: %"PRIu32", lf_sec: %"PRIu64", "
			   "fmt:%"PRIu32" next=0x%"PRIx64" (%d dirents).",
			   be16_to_cpu(leaf->lf_depth), be16_to_cpu(leaf->lf_entries),
		           be64_to_cpu(leaf->lf_inode), be32_to_cpu(leaf->lf_dist),
		           be32_to_cpu(leaf->lf_nsec), be64_to_cpu(leaf->lf_sec),
			   leaf->lf_dirent_format,
			   leaf->lf_next,
			   ind->ii[0].dirents);

	start_line = line;
	for (d = start_row[dmode]; d < ind->ii[0].dirents; d++) {
		if (termlines && d >= termlines - start_line - 2
		    + start_row[dmode])
			break;
		total_dirents++;
		if (ind->ii[0].dirents >= 1) {
			eol(3);
			if (termlines) {
				if (edit_row[dmode] >=0 &&
				    line - start_line - 1 ==
				    edit_row[dmode] - start_row[dmode]) {
					COLORS_HIGHLIGHT;
					sprintf(estring, "%"PRIx64, ind->ii[0].dirent[d].inum.in_addr);
					strcpy(edit_fmt, "%llx");
				}
			}
			print_gfs2("%d/%d [%08"PRIX32"] %"PRIu64"/%"PRIu64" (0x%"PRIx64"/0x%"PRIx64") +%"PRIu16": ",
				   total_dirents, d + 1,
				   ind->ii[0].dirent[d].hash,
				   ind->ii[0].dirent[d].inum.in_formal_ino,
				   ind->ii[0].dirent[d].inum.in_addr,
				   ind->ii[0].dirent[d].inum.in_formal_ino,
				   ind->ii[0].dirent[d].inum.in_addr,
				   ind->ii[0].dirent[d].rahead
			);
		}
		print_inode_type(ind->ii[0].dirent[d].type);
		print_gfs2(" %s", ind->ii[0].dirent[d].filename);
		if (termlines) {
			if (edit_row[dmode] >= 0 &&
			    line - start_line - 1 == edit_row[dmode] -
			    start_row[dmode])
				COLORS_NORMAL;
		}
	}
	if (line >= 4)
		last_entry_onscreen[dmode] = line - 4;
	eol(0);
	end_row[dmode] = ind->ii[0].dirents;
	if (end_row[dmode] < last_entry_onscreen[dmode])
		end_row[dmode] = last_entry_onscreen[dmode];
	return 0;
}

static void print_block_details(struct iinfo *ind, int level, int cur_height,
				int pndx, uint64_t file_offset)
{
	struct iinfo *more_indir;
	int more_ind;
	char *tmpbuf;
	uint64_t thisblk;

	thisblk = ind->ii[pndx].block;
	more_indir = malloc(sizeof(struct iinfo));
	if (!more_indir) {
		fprintf(stderr, "Out of memory in function "
			"display_indirect\n");
		return;
	}
	tmpbuf = malloc(sbd.sd_bsize);
	if (!tmpbuf) {
		fprintf(stderr, "Out of memory in function "
			"display_indirect\n");
		free(more_indir);
		return;
	}
	while (thisblk) {
		/* read in the desired block */
		if (pread(sbd.device_fd, tmpbuf, sbd.sd_bsize, thisblk * sbd.sd_bsize) != sbd.sd_bsize) {
			fprintf(stderr, "bad read: %s from %s:%d: block %"PRIu64
				" (0x%"PRIx64")\n", strerror(errno), __FUNCTION__,
				__LINE__, ind->ii[pndx].block, ind->ii[pndx].block);
			exit(-1);
		}
		thisblk = 0;
		memset(more_indir, 0, sizeof(struct iinfo));
		if (S_ISDIR(be32_to_cpu(di->di_mode)) && level == be16_to_cpu(di->di_height)) {
			thisblk = do_leaf_extended(tmpbuf, more_indir);
			display_leaf(more_indir);
		} else {
			int x;

			for (x = 0; x < 512; x++) {
				memcpy(&more_indir->ii[x].mp,
				       &ind->ii[pndx].mp,
				       sizeof(struct metapath));
				more_indir->ii[x].mp.mp_list[cur_height+1] = x;
			}
			more_ind = _do_indirect_extended(tmpbuf, more_indir,
							 cur_height + 1);
			display_indirect(more_indir, more_ind, level + 1,
					 file_offset);
		}
		if (thisblk) {
			eol(0);
			if (termlines)
				move(line,9);
			print_gfs2("Continuation block 0x%"PRIx64" / %"PRId64,
				   thisblk, thisblk);
		}
	}
	free(tmpbuf);
	free(more_indir);
}

static void gfs_jindex_print(struct gfs_jindex *ji)
{
	print_it("  ji_addr", "%"PRIu64, "0x%"PRIx64, be64_to_cpu(ji->ji_addr));
	print_it("  ji_nsegment", "%"PRIu32, "0x%"PRIx32, be32_to_cpu(ji->ji_nsegment));
	print_it("  ji_pad", "%"PRIu32, "0x%"PRIx32, be32_to_cpu(ji->ji_pad));
}

static int print_gfs_jindex(struct gfs2_inode *dij)
{
	int error, start_line;
	struct gfs_jindex *ji;
	char jbuf[sizeof(struct gfs_jindex)];

	start_line = line;
	print_gfs2("Journal index entries found: %"PRIu64".",
		   dij->i_size / sizeof(struct gfs_jindex));
	eol(0);
	lines_per_row[dmode] = 4;
	for (print_entry_ndx=0; ; print_entry_ndx++) {
		error = gfs2_readi(dij, (void *)&jbuf,
				   print_entry_ndx*sizeof(struct gfs_jindex),
				   sizeof(struct gfs_jindex));
		ji = (struct gfs_jindex *)jbuf;
		if (!error) /* end of file */
			break;
		if (!termlines ||
		    (print_entry_ndx >= start_row[dmode] &&
		     ((print_entry_ndx - start_row[dmode])+1) *
		     lines_per_row[dmode] <= termlines - start_line - 2)) {
			if (edit_row[dmode] == print_entry_ndx) {
				COLORS_HIGHLIGHT;
				strcpy(efield, "ji_addr");
				sprintf(estring, "%"PRIx64, be64_to_cpu(ji->ji_addr));
			}
			print_gfs2("Journal #%d", print_entry_ndx);
			eol(0);
			if (edit_row[dmode] == print_entry_ndx)
				COLORS_NORMAL;
			gfs_jindex_print(ji);
			last_entry_onscreen[dmode] = print_entry_ndx;
		}
	}
	end_row[dmode] = print_entry_ndx;
	return error;
}

static int print_gfs2_jindex(void)
{
	int d, error;
	struct lgfs2_log_header head;
	struct gfs2_inode *ip;

	for (d = 0; d < indirect->ii[0].dirents; d++) {
		if (strncmp(indirect->ii[0].dirent[d].filename, "journal", 7))
			continue;
		ip = lgfs2_inode_read(&sbd, indirect->ii[0].dirent[d].inum.in_addr);
		print_gfs2("%s: 0x%-5"PRIx64" %"PRIu64"MB ",
			   indirect->ii[0].dirent[d].filename,
			   indirect->ii[0].dirent[d].inum.in_addr,
			   ip->i_size / 1048576);
		error = lgfs2_find_jhead(ip, &head);
		if (error) {
			print_gfs2("corrupt.");
		} else {
			if (head.lh_flags & GFS2_LOG_HEAD_UNMOUNT)
				print_gfs2("clean.");
			else
				print_gfs2("dirty.");
		}
		eol(0);
		inode_put(&ip);
	}
	return 0;
}

static int parse_rindex(struct gfs2_inode *dip, int print_rindex)
{
	int error, start_line;
	struct gfs2_rindex ri;
	char highlighted_addr[32];
	struct rgrp_tree rg = {0};

	start_line = line;
	print_gfs2("RG index entries found: %"PRIu64".",
	           dip->i_size / sizeof(struct gfs2_rindex));
	eol(0);
	lines_per_row[dmode] = 6;
	memset(highlighted_addr, 0, sizeof(highlighted_addr));

	for (print_entry_ndx=0; ; print_entry_ndx++) {
		uint64_t roff;

		roff = print_entry_ndx * sizeof(struct gfs2_rindex);

		error = gfs2_readi(dip, &ri, roff, sizeof(struct gfs2_rindex));
		if (!error) /* end of file */
			break;
		lgfs2_rindex_in(&rg, &ri);
		if (!termlines ||
			(print_entry_ndx >= start_row[dmode] &&
			 ((print_entry_ndx - start_row[dmode])+1) * lines_per_row[dmode] <=
			 termlines - start_line - 2)) {
			if (edit_row[dmode] == print_entry_ndx) {
				COLORS_HIGHLIGHT;
				sprintf(highlighted_addr, "%"PRIx64, rg.rt_addr);
			}
			print_gfs2("RG #%d", print_entry_ndx);
			if (!print_rindex)
				print_gfs2(" located at: %"PRIu64" (0x%"PRIx64")",
					   rg.rt_addr, rg.rt_addr);
			eol(0);
			if (edit_row[dmode] == print_entry_ndx)
				COLORS_NORMAL;
			if (print_rindex)
				lgfs2_rindex_print(&ri);
			else {
				struct gfs2_rgrp r = {0};
				ssize_t ret;

				ret = pread(sbd.device_fd, &r, sizeof(r), rg.rt_addr * sbd.sd_bsize);
				if (ret != sizeof(r)) {
					perror("Failed to read resource group");
				} else if (sbd.gfs1) {
					gfs_rgrp_print(&r);
				} else {
					lgfs2_rgrp_print(&r);
				}
			}
			last_entry_onscreen[dmode] = print_entry_ndx;
		}
	}
	strcpy(estring, highlighted_addr);
	end_row[dmode] = print_entry_ndx;
	return error;
}

static int print_inum(struct gfs2_inode *dii)
{
	uint64_t inodenum;
	__be64 inum;
	int rc;
	
	rc = gfs2_readi(dii, (void *)&inum, 0, sizeof(inum));
	if (!rc) {
		print_gfs2("The inum file is empty.");
		eol(0);
		return 0;
	}
	if (rc != sizeof(inum)) {
		print_gfs2("Error reading inum file.");
		eol(0);
		return -1;
	}
	inodenum = be64_to_cpu(inum);
	print_gfs2("Next inode num = %"PRId64" (0x%"PRIx64")", inodenum, inodenum);
	eol(0);
	return 0;
}

static int print_statfs(struct gfs2_inode *dis)
{
	struct gfs2_statfs_change sc;
	int rc;

	rc = gfs2_readi(dis, (void *)&sc, 0, sizeof(sc));
	if (!rc) {
		print_gfs2("The statfs file is empty.");
		eol(0);
		return 0;
	}
	if (rc != sizeof(sc)) {
		print_gfs2("Error reading statfs file.");
		eol(0);
		return -1;
	}
	print_gfs2("statfs file contents:");
	eol(0);
	lgfs2_statfs_change_print(&sc);
	return 0;
}

static int print_quota(struct gfs2_inode *diq)
{
	struct gfs2_quota q;
	int i, error;

	print_gfs2("quota file contents:");
	eol(0);
	print_gfs2("quota entries found: %"PRIu64".", diq->i_size / sizeof(q));
	eol(0);
	for (i=0; ; i++) {
		error = gfs2_readi(diq, &q, i * sizeof(q), sizeof(q));
		if (!error)
			break;
		if (error != sizeof(q)) {
			print_gfs2("Error reading quota file.");
			eol(0);
			return -1;
		}
		print_gfs2("Entry #%d", i + 1);
		eol(0);
		lgfs2_quota_print(&q);
	}
	return 0;
}

int display_extended(void)
{
	struct gfs2_inode *tmp_inode;
	struct gfs2_buffer_head *tmp_bh;

	dsplines = termlines - line - 1;
	/* Display any indirect pointers that we have. */
	if (block_is_rindex(block)) {
		tmp_bh = bread(&sbd, block);
		tmp_inode = lgfs2_inode_get(&sbd, tmp_bh);
		if (tmp_inode == NULL)
			return -1;
		parse_rindex(tmp_inode, TRUE);
		inode_put(&tmp_inode);
		brelse(tmp_bh);
	} else if (block_is_journals(block)) {
		if (sbd.gfs1)
			block = sbd.sd_jindex_di.in_addr;
		else
			block = masterblock("jindex");
		print_gfs2_jindex();
	} else if (has_indirect_blocks() && !indirect_blocks &&
		 !display_leaf(indirect))
		return -1;
	else if (display_indirect(indirect, indirect_blocks, 0, 0) == 0)
		return -1;
	else if (block_is_rgtree(block)) {
		if (sbd.gfs1)
			tmp_bh = bread(&sbd, sbd.sd_rindex_di.in_addr);
		else
			tmp_bh = bread(&sbd, masterblock("rindex"));
		tmp_inode = lgfs2_inode_get(&sbd, tmp_bh);
		if (tmp_inode == NULL)
			return -1;
		parse_rindex(tmp_inode, FALSE);
		inode_put(&tmp_inode);
		brelse(tmp_bh);
	} else if (block_is_jindex(block)) {
		tmp_bh = bread(&sbd, block);
		tmp_inode = lgfs2_inode_get(&sbd, tmp_bh);
		if (tmp_inode == NULL)
			return -1;
		print_gfs_jindex(tmp_inode);
		inode_put(&tmp_inode);
		brelse(tmp_bh);
	}
	else if (block_is_inum_file(block)) {
		tmp_bh = bread(&sbd, block);
		tmp_inode = lgfs2_inode_get(&sbd, tmp_bh);
		if (tmp_inode == NULL)
			return -1;
		print_inum(tmp_inode);
		inode_put(&tmp_inode);
		brelse(tmp_bh);
	}
	else if (block_is_statfs_file(block)) {
		tmp_bh = bread(&sbd, block);
		tmp_inode = lgfs2_inode_get(&sbd, tmp_bh);
		if (tmp_inode == NULL)
			return -1;
		print_statfs(tmp_inode);
		inode_put(&tmp_inode);
		brelse(tmp_bh);
	}
	else if (block_is_quota_file(block)) {
		tmp_bh = bread(&sbd, block);
		tmp_inode = lgfs2_inode_get(&sbd, tmp_bh);
		if (tmp_inode == NULL)
			return -1;
		print_quota(tmp_inode);
		inode_put(&tmp_inode);
		brelse(tmp_bh);
	}
	return 0;
}

