#include <stdint.h>
#include <string.h>
#include <uuid.h>
#include "libgfs2.h"
#include "clusterautoconfig.h"

#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))
#define SYM(x) { x, #x, 0 },
#define PREFIX_SYM(p,x) { p##x, #p #x, sizeof #p - 1 },

const struct lgfs2_symbolic lgfs2_metatypes[] = {
SYM(GFS2_METATYPE_NONE)
SYM(GFS2_METATYPE_SB)
SYM(GFS2_METATYPE_RG)
SYM(GFS2_METATYPE_RB)
SYM(GFS2_METATYPE_DI)
SYM(GFS2_METATYPE_IN)
SYM(GFS2_METATYPE_LF)
SYM(GFS2_METATYPE_JD)
SYM(GFS2_METATYPE_LH)
SYM(GFS2_METATYPE_LD)
SYM(GFS2_METATYPE_LB)
SYM(GFS2_METATYPE_EA)
SYM(GFS2_METATYPE_ED)
SYM(GFS2_METATYPE_QC)
};

const unsigned lgfs2_metatype_size = ARRAY_SIZE(lgfs2_metatypes);

const struct lgfs2_symbolic lgfs2_metaformats[] = {
SYM(GFS2_FORMAT_NONE)
SYM(GFS2_FORMAT_SB)
SYM(GFS2_FORMAT_RG)
SYM(GFS2_FORMAT_RB)
SYM(GFS2_FORMAT_DI)
SYM(GFS2_FORMAT_IN)
SYM(GFS2_FORMAT_LF)
SYM(GFS2_FORMAT_JD)
SYM(GFS2_FORMAT_LH)
SYM(GFS2_FORMAT_LD)
SYM(GFS2_FORMAT_LB)
SYM(GFS2_FORMAT_EA)
SYM(GFS2_FORMAT_ED)
SYM(GFS2_FORMAT_QC)
SYM(GFS2_FORMAT_RI)
SYM(GFS2_FORMAT_DE)
SYM(GFS2_FORMAT_QU)
};

const unsigned lgfs2_metaformat_size = ARRAY_SIZE(lgfs2_metaformats);

const struct lgfs2_symbolic lgfs2_di_flags[] = {
SYM(GFS2_DIF_JDATA)
SYM(GFS2_DIF_EXHASH)
SYM(GFS2_DIF_UNUSED)
SYM(GFS2_DIF_EA_INDIRECT)
SYM(GFS2_DIF_DIRECTIO)
SYM(GFS2_DIF_IMMUTABLE)
SYM(GFS2_DIF_APPENDONLY)
SYM(GFS2_DIF_NOATIME)
SYM(GFS2_DIF_SYNC)
SYM(GFS2_DIF_SYSTEM)
SYM(GFS2_DIF_TRUNC_IN_PROG)
SYM(GFS2_DIF_INHERIT_DIRECTIO)
SYM(GFS2_DIF_INHERIT_JDATA)
};

const unsigned lgfs2_di_flag_size = ARRAY_SIZE(lgfs2_di_flags);

const struct lgfs2_symbolic lgfs2_lh_flags[] = {
PREFIX_SYM(GFS2_LOG_HEAD_, UNMOUNT)
PREFIX_SYM(GFS2_LOG_HEAD_FLUSH_, NORMAL)
PREFIX_SYM(GFS2_LOG_HEAD_FLUSH_, SYNC)
PREFIX_SYM(GFS2_LOG_HEAD_FLUSH_, SHUTDOWN)
PREFIX_SYM(GFS2_LOG_HEAD_FLUSH_, FREEZE)
PREFIX_SYM(GFS2_LOG_HEAD_, RECOVERY)
PREFIX_SYM(GFS2_LOG_HEAD_, USERSPACE)

/* Log flush callers */
PREFIX_SYM(GFS2_LFC_, SHUTDOWN)
PREFIX_SYM(GFS2_LFC_, JDATA_WPAGES)
PREFIX_SYM(GFS2_LFC_, SET_FLAGS)
PREFIX_SYM(GFS2_LFC_, AIL_EMPTY_GL)
PREFIX_SYM(GFS2_LFC_, AIL_FLUSH)
PREFIX_SYM(GFS2_LFC_, RGRP_GO_SYNC)
PREFIX_SYM(GFS2_LFC_, INODE_GO_SYNC)
PREFIX_SYM(GFS2_LFC_, INODE_GO_INVAL)
PREFIX_SYM(GFS2_LFC_, FREEZE_GO_SYNC)
PREFIX_SYM(GFS2_LFC_, KILL_SB)
PREFIX_SYM(GFS2_LFC_, DO_SYNC)
PREFIX_SYM(GFS2_LFC_, INPLACE_RESERVE)
PREFIX_SYM(GFS2_LFC_, WRITE_INODE)
PREFIX_SYM(GFS2_LFC_, MAKE_FS_RO)
PREFIX_SYM(GFS2_LFC_, SYNC_FS)
PREFIX_SYM(GFS2_LFC_, EVICT_INODE)
PREFIX_SYM(GFS2_LFC_, TRANS_END)
PREFIX_SYM(GFS2_LFC_, LOGD_JFLUSH_REQD)
PREFIX_SYM(GFS2_LFC_, LOGD_AIL_FLUSH_REQD)
};

const unsigned int lgfs2_lh_flag_size = ARRAY_SIZE(lgfs2_lh_flags);

const struct lgfs2_symbolic lgfs2_ld_types[] = {
SYM(GFS2_LOG_DESC_METADATA)
SYM(GFS2_LOG_DESC_REVOKE)
SYM(GFS2_LOG_DESC_JDATA)
};

const unsigned int lgfs2_ld_type_size = ARRAY_SIZE(lgfs2_ld_types);

const struct lgfs2_symbolic lgfs2_ld1_types[] = {
SYM(GFS_LOG_DESC_METADATA)
SYM(GFS_LOG_DESC_IUL)
SYM(GFS_LOG_DESC_IDA)
SYM(GFS_LOG_DESC_Q)
SYM(GFS_LOG_DESC_LAST)
};

const unsigned int lgfs2_ld1_type_size = ARRAY_SIZE(lgfs2_ld1_types);

#undef SYM




#define F(f,...)  { .name = #f, \
		    .offset = offsetof(struct STRUCT, f), \
		    .length = sizeof(((struct STRUCT *)(0))->f), \
		    __VA_ARGS__ },
#define FP(f,...) F(f, .flags = LGFS2_MFF_POINTER, __VA_ARGS__)
#define RF(f) F(f, .flags = LGFS2_MFF_RESERVED)
#define RFP(f,...) F(f, .flags = LGFS2_MFF_POINTER|LGFS2_MFF_RESERVED, __VA_ARGS__)


#define MH(f) F(f.mh_magic) \
	      F(f.mh_type, .flags = LGFS2_MFF_ENUM, .symtab=lgfs2_metatypes, .nsyms=ARRAY_SIZE(lgfs2_metatypes)) \
	      RF(f.__pad0) \
	      F(f.mh_format, .flags = LGFS2_MFF_ENUM, .symtab=lgfs2_metaformats, .nsyms=ARRAY_SIZE(lgfs2_metaformats)) \
	      F(f.mh_jid)

#define IN(f,...) F(f.no_formal_ino) \
		  FP(f.no_addr, __VA_ARGS__)

#define INR(f,...) RF(f.no_formal_ino) \
		   RFP(f.no_addr, __VA_ARGS__)
#define ANY_COMMON_BLOCK (1 << LGFS2_MT_DIR_LEAF) | \
			 (1 << LGFS2_MT_JRNL_DATA) | \
			 (1 << LGFS2_MT_EA_ATTR) | \
			 (1 << LGFS2_MT_EA_DATA) | \
			 (1 << LGFS2_MT_DATA)

#define ANY_GFS2_BLOCK (1 << LGFS2_MT_GFS2_DINODE) | \
		       (1 << LGFS2_MT_GFS2_INDIRECT) | \
		       (1 << LGFS2_MT_GFS2_LOG_HEADER) | \
		       (1 << LGFS2_MT_GFS2_LOG_DESC) | \
		       (1 << LGFS2_MT_GFS2_LOG_BLOCK) | \
		       ANY_COMMON_BLOCK

#define ANY_GFS_BLOCK (1 << LGFS2_MT_GFS_DINODE) | \
		      (1 << LGFS2_MT_GFS_INDIRECT) | \
		      ANY_COMMON_BLOCK

#undef STRUCT
#define STRUCT gfs2_sb

static const struct lgfs2_metafield gfs2_sb_fields[] = {
MH(sb_header)
F(sb_fs_format)
F(sb_multihost_format)
RF(__pad0)
F(sb_bsize, .flags = LGFS2_MFF_BYTES)
F(sb_bsize_shift, .flags = LGFS2_MFF_BYTES|LGFS2_MFF_SHIFT)
RF(__pad1)
IN(sb_master_dir, .points_to = (1 << LGFS2_MT_GFS2_DINODE))
INR(__pad2, .points_to = (1 << LGFS2_MT_GFS2_DINODE))
IN(sb_root_dir, .points_to = (1 << LGFS2_MT_GFS2_DINODE))
F(sb_lockproto, .flags = LGFS2_MFF_STRING)
F(sb_locktable, .flags = LGFS2_MFF_STRING)
INR(__pad3, .points_to = (1 << LGFS2_MT_GFS2_DINODE))
INR(__pad4, .points_to = (1 << LGFS2_MT_GFS2_DINODE))
F(sb_uuid, .flags = LGFS2_MFF_UUID)
};

#undef STRUCT
#define STRUCT gfs_sb

static const struct lgfs2_metafield gfs_sb_fields[] = {
MH(sb_header)
F(sb_fs_format)
F(sb_multihost_format)
F(sb_flags)
F(sb_bsize, .flags = LGFS2_MFF_BYTES)
F(sb_bsize_shift, .flags = LGFS2_MFF_BYTES|LGFS2_MFF_SHIFT)
F(sb_seg_size, .flags = LGFS2_MFF_FSBLOCKS)
IN(sb_jindex_di, .points_to = (1 << LGFS2_MT_GFS_DINODE))
IN(sb_rindex_di, .points_to = (1 << LGFS2_MT_GFS_DINODE))
IN(sb_root_di, .points_to = (1 << LGFS2_MT_GFS_DINODE))
F(sb_lockproto, .flags = LGFS2_MFF_STRING)
F(sb_locktable, .flags = LGFS2_MFF_STRING)
IN(sb_quota_di, .points_to = (1 << LGFS2_MT_GFS_DINODE))
IN(sb_license_di, .points_to = (1 << LGFS2_MT_GFS_DINODE))
RF(sb_reserved)
};

#undef STRUCT
#define STRUCT gfs2_rindex

static const struct lgfs2_metafield gfs2_rindex_fields[] = {
FP(ri_addr, .points_to = (1 << LGFS2_MT_GFS2_RGRP))
F(ri_length, .flags = LGFS2_MFF_FSBLOCKS)
RF(__pad)
FP(ri_data0, .points_to = ANY_GFS2_BLOCK|(1 << LGFS2_MT_FREE))
F(ri_data, .flags = LGFS2_MFF_FSBLOCKS)
F(ri_bitbytes, .flags = LGFS2_MFF_BYTES)
F(ri_reserved)
};

#undef STRUCT
#define STRUCT gfs2_rgrp

static const struct lgfs2_metafield gfs2_rgrp_fields[] = {
MH(rg_header)
F(rg_flags)
F(rg_free, .flags = LGFS2_MFF_FSBLOCKS)
F(rg_dinodes, .flags = LGFS2_MFF_FSBLOCKS)
FP(rg_skip, .points_to = (1 << LGFS2_MT_GFS2_RGRP))
F(rg_igeneration)
FP(rg_data0, .points_to = ANY_GFS2_BLOCK|(1 << LGFS2_MT_FREE))
F(rg_data, .flags = LGFS2_MFF_FSBLOCKS)
F(rg_bitbytes, .flags = LGFS2_MFF_BYTES)
F(rg_crc, .flags = LGFS2_MFF_CHECK)
RF(rg_reserved)
};

#undef STRUCT
#define STRUCT gfs_rgrp

static const struct lgfs2_metafield gfs_rgrp_fields[] = {
MH(rg_header)
F(rg_flags)
F(rg_free, .flags = LGFS2_MFF_FSBLOCKS)
F(rg_useddi, .flags = LGFS2_MFF_FSBLOCKS)
F(rg_freedi, .flags = LGFS2_MFF_FSBLOCKS)
IN(rg_freedi_list, .points_to = (1 << LGFS2_MT_GFS_DINODE))
F(rg_usedmeta, .flags = LGFS2_MFF_FSBLOCKS)
F(rg_freemeta, .flags = LGFS2_MFF_FSBLOCKS)
RF(rg_reserved)
};

#undef STRUCT
struct gfs2_rgrp_bitmap { struct gfs2_meta_header rb_header; };
#define STRUCT gfs2_rgrp_bitmap

static const struct lgfs2_metafield gfs2_rgrp_bitmap_fields[] = {
MH(rb_header)
};

#undef STRUCT
#define STRUCT gfs2_dinode

static const struct lgfs2_metafield gfs2_dinode_fields[] = {
MH(di_header)
IN(di_num, .points_to = (1 << LGFS2_MT_GFS2_DINODE))
F(di_mode, .flags = LGFS2_MFF_MODE)
F(di_uid, .flags = LGFS2_MFF_UID)
F(di_gid, .flags = LGFS2_MFF_GID)
F(di_nlink)
F(di_size, .flags = LGFS2_MFF_BYTES)
F(di_blocks, .flags = LGFS2_MFF_FSBLOCKS)
F(di_atime, .flags = LGFS2_MFF_SECS)
F(di_mtime, .flags = LGFS2_MFF_SECS)
F(di_ctime, .flags = LGFS2_MFF_SECS)
F(di_major, .flags = LGFS2_MFF_MAJOR)
F(di_minor, .flags = LGFS2_MFF_MINOR)
FP(di_goal_meta, .points_to = ANY_GFS2_BLOCK | (1 << LGFS2_MT_FREE))
FP(di_goal_data, .points_to = ANY_GFS2_BLOCK | (1 << LGFS2_MT_FREE))
F(di_generation)
F(di_flags, .flags = LGFS2_MFF_MASK, .symtab=lgfs2_di_flags, .nsyms=ARRAY_SIZE(lgfs2_di_flags))
F(di_payload_format)
RF(__pad1)
F(di_height)
RF(__pad2)
RF(__pad3)
F(di_depth)
F(di_entries)
INR(__pad4, .points_to = (1 << LGFS2_MT_GFS2_DINODE))
FP(di_eattr, .points_to = (1 << LGFS2_MT_EA_ATTR)|(1 << LGFS2_MT_GFS2_INDIRECT))
F(di_atime_nsec, .flags = LGFS2_MFF_NSECS)
F(di_mtime_nsec, .flags = LGFS2_MFF_NSECS)
F(di_ctime_nsec, .flags = LGFS2_MFF_NSECS)
RF(di_reserved)
};

#undef STRUCT
#define STRUCT gfs_dinode

static const struct lgfs2_metafield gfs_dinode_fields[] = {
MH(di_header)
IN(di_num, .points_to = (1 << LGFS2_MT_GFS_DINODE))
F(di_mode, .flags = LGFS2_MFF_MODE)
F(di_uid, .flags = LGFS2_MFF_UID)
F(di_gid, .flags = LGFS2_MFF_GID)
F(di_nlink)
F(di_size, .flags = LGFS2_MFF_BYTES)
F(di_blocks, .flags = LGFS2_MFF_FSBLOCKS)
F(di_atime, .flags = LGFS2_MFF_SECS)
F(di_mtime, .flags = LGFS2_MFF_SECS)
F(di_ctime, .flags = LGFS2_MFF_SECS)
F(di_major, .flags = LGFS2_MFF_MAJOR)
F(di_minor, .flags = LGFS2_MFF_MINOR)
FP(di_rgrp, .points_to = LGFS2_MT_GFS_RGRP)
FP(di_goal_rgrp, .points_to = LGFS2_MT_GFS_RGRP)
F(di_goal_dblk)
F(di_goal_mblk)
F(di_flags, .flags = LGFS2_MFF_MASK, .symtab=lgfs2_di_flags, .nsyms=ARRAY_SIZE(lgfs2_di_flags))
F(di_payload_format)
F(di_type)
F(di_height)
F(di_incarn)
F(di_pad)
F(di_depth)
F(di_entries)
INR(di_next_unused, .points_to = (1 << LGFS2_MT_GFS_DINODE))
FP(di_eattr, .points_to = (1 << LGFS2_MT_EA_ATTR)|(1 << LGFS2_MT_GFS_INDIRECT))
F(di_reserved)
};

#undef STRUCT
struct gfs2_indirect { struct gfs2_meta_header in_header; };
#define STRUCT gfs2_indirect

static const struct lgfs2_metafield gfs2_indirect_fields[] = {
MH(in_header)
};

#undef STRUCT
#define STRUCT gfs_indirect

static const struct lgfs2_metafield gfs_indirect_fields[] = {
MH(in_header)
RF(in_reserved)
};

#undef STRUCT
#define STRUCT gfs2_leaf

static const struct lgfs2_metafield gfs2_leaf_fields[] = {
MH(lf_header)
F(lf_depth)
F(lf_entries)
F(lf_dirent_format)
F(lf_next)
#ifdef GFS2_HAS_LEAF_HINTS
FP(lf_inode, .points_to = (1 << LGFS2_MT_GFS2_DINODE))
F(lf_dist)
F(lf_nsec, .flags = LGFS2_MFF_NSECS)
F(lf_sec, .flags = LGFS2_MFF_SECS)
RF(lf_reserved2)
#else
RF(lf_reserved)
#endif
};

#undef STRUCT
struct gfs2_jrnl_data { struct gfs2_meta_header jd_header; };
#define STRUCT gfs2_jrnl_data

static const struct lgfs2_metafield gfs2_jdata_fields[] = {
MH(jd_header)
};

#undef STRUCT
#define STRUCT gfs2_log_header

static const struct lgfs2_metafield gfs2_log_header_fields[] = {
MH(lh_header)
F(lh_sequence)
F(lh_flags, .flags = LGFS2_MFF_MASK, .symtab = lgfs2_lh_flags, .nsyms = ARRAY_SIZE(lgfs2_lh_flags))
F(lh_tail)
F(lh_blkno)
F(lh_hash, .flags = LGFS2_MFF_CHECK)
F(lh_crc, .flags = LGFS2_MFF_CHECK)
F(lh_nsec, .flags = LGFS2_MFF_NSECS)
F(lh_sec, .flags = LGFS2_MFF_SECS)
FP(lh_addr, .points_to = (1 << LGFS2_MT_GFS2_LOG_BLOCK))
FP(lh_jinode, .points_to = (1 << LGFS2_MT_GFS2_DINODE))
FP(lh_statfs_addr, .points_to = (1 << LGFS2_MT_GFS2_DINODE))
FP(lh_quota_addr, .points_to = (1 << LGFS2_MT_GFS2_DINODE))
F(lh_local_total, .flags = LGFS2_MFF_FSBLOCKS)
F(lh_local_free, .flags = LGFS2_MFF_FSBLOCKS)
F(lh_local_dinodes, .flags = LGFS2_MFF_FSBLOCKS)
};

#undef STRUCT
#define STRUCT gfs_log_header

static const struct lgfs2_metafield gfs_log_header_fields[] = {
MH(lh_header)
F(lh_flags)
RF(lh_pad)
F(lh_first)
F(lh_sequence)
F(lh_tail)
F(lh_last_dump)
RF(lh_reserved)
};

#undef STRUCT
#define STRUCT gfs2_log_descriptor

static const struct lgfs2_metafield gfs2_log_desc_fields[] = {
MH(ld_header)
F(ld_type, .flags = LGFS2_MFF_ENUM, .symtab = lgfs2_ld_types, .nsyms = ARRAY_SIZE(lgfs2_ld_types))
F(ld_length, .flags = LGFS2_MFF_FSBLOCKS)
F(ld_data1)
F(ld_data2)
RF(ld_reserved)
};

#undef STRUCT
#define STRUCT gfs_log_descriptor

static const struct lgfs2_metafield gfs_log_desc_fields[] = {
MH(ld_header)
F(ld_type, .flags = LGFS2_MFF_ENUM, .symtab = lgfs2_ld1_types, .nsyms = ARRAY_SIZE(lgfs2_ld1_types))
F(ld_length, .flags = LGFS2_MFF_FSBLOCKS)
F(ld_data1)
F(ld_data2)
RF(ld_reserved)
};

#undef STRUCT
struct gfs2_log_block { struct gfs2_meta_header lb_header; };
#define STRUCT gfs2_log_block

static const struct lgfs2_metafield gfs2_log_block_fields[] = {
MH(lb_header)
};

#undef STRUCT
struct gfs2_ea_attr { struct gfs2_meta_header ea_header; };
#define STRUCT gfs2_ea_attr

static const struct lgfs2_metafield gfs2_ea_attr_fields[] = {
MH(ea_header)
};

#undef STRUCT
struct gfs2_ea_data { struct gfs2_meta_header ed_header; };
#define STRUCT gfs2_ea_data

static const struct lgfs2_metafield gfs2_ea_data_fields[] = {
MH(ed_header)
};

#undef STRUCT
#define STRUCT gfs2_quota_change

static const struct lgfs2_metafield gfs2_quota_change_fields[] = {
F(qc_change, .flags = LGFS2_MFF_FSBLOCKS)
F(qc_flags)
F(qc_id)
};

#undef STRUCT
#define STRUCT gfs2_dirent

static const struct lgfs2_metafield gfs2_dirent_fields[] = {
IN(de_inum, .points_to = (1 << LGFS2_MT_GFS_DINODE)|(1 << LGFS2_MT_GFS2_DINODE))
F(de_hash, .flags = LGFS2_MFF_CHECK)
F(de_rec_len, .flags = LGFS2_MFF_BYTES)
F(de_name_len, .flags = LGFS2_MFF_BYTES)
F(de_type)
F(de_rahead)
F(de_cookie)
RF(pad3)
};

#undef STRUCT
#define STRUCT gfs2_ea_header

static const struct lgfs2_metafield gfs2_ea_header_fields[] = {
F(ea_rec_len, .flags = LGFS2_MFF_BYTES)
F(ea_data_len, .flags = LGFS2_MFF_BYTES)
F(ea_name_len, .flags = LGFS2_MFF_BYTES)
F(ea_type)
F(ea_flags)
F(ea_num_ptrs)
RF(__pad)
};

#undef STRUCT
#define STRUCT gfs2_inum_range

static const struct lgfs2_metafield gfs2_inum_range_fields[] = {
F(ir_start)
F(ir_length)
};

#undef STRUCT
#define STRUCT gfs2_statfs_change

static const struct lgfs2_metafield gfs2_statfs_change_fields[] = {
F(sc_total, .flags = LGFS2_MFF_FSBLOCKS)
F(sc_free, .flags = LGFS2_MFF_FSBLOCKS)
F(sc_dinodes, .flags = LGFS2_MFF_FSBLOCKS)
};

#undef STRUCT
#define STRUCT gfs_jindex

static const struct lgfs2_metafield gfs_jindex_fields[] = {
FP(ji_addr, .points_to = (1 << LGFS2_MT_DATA))
F(ji_nsegment)
RF(ji_pad)
RF(ji_reserved)
};

#undef STRUCT
struct gfs_block_tag {
	uint64_t bt_blkno;      /* inplace block number */
	uint32_t bt_flags;      /* ?? */
	uint32_t bt_pad;
};
#define STRUCT gfs_block_tag

static const struct lgfs2_metafield gfs_block_tag_fields[] = {
FP(bt_blkno, .points_to = ANY_GFS_BLOCK)
RF(bt_flags)
RF(bt_pad)
};

const struct lgfs2_metadata lgfs2_metadata[] = {
	[LGFS2_MT_GFS2_SB] = {
		.versions = LGFS2_MD_GFS2,
		.header = 1,
		.mh_type = GFS2_METATYPE_SB,
		.mh_format = GFS2_FORMAT_SB,
		.name = "gfs2_sb",
		.display = "superblock",
		.fields = gfs2_sb_fields,
		.nfields = ARRAY_SIZE(gfs2_sb_fields),
		.size = sizeof(struct gfs2_sb),
	},
	[LGFS2_MT_GFS_SB] = {
		.versions = LGFS2_MD_GFS1,
		.header = 1,
		.mh_type = GFS2_METATYPE_SB,
		.mh_format = GFS_FORMAT_SB,
		.name = "gfs_sb",
		.display = "superblock",
		.fields = gfs_sb_fields,
		.nfields = ARRAY_SIZE(gfs_sb_fields),
		.size = sizeof(struct gfs_sb),
	},
	[LGFS2_MT_RINDEX] = {
		.versions = LGFS2_MD_GFS1 | LGFS2_MD_GFS2,
		.name = "gfs2_rindex",
		.display = "resource group index",
		.fields = gfs2_rindex_fields,
		.nfields = ARRAY_SIZE(gfs2_rindex_fields),
		.size = sizeof(struct gfs2_rindex),
	},
	[LGFS2_MT_GFS2_RGRP] = {
		.versions = LGFS2_MD_GFS2,
		.header = 1,
		.mh_type = GFS2_METATYPE_RG,
		.mh_format = GFS2_FORMAT_RG,
		.name = "gfs2_rgrp",
		.display = "resource group",
		.fields = gfs2_rgrp_fields,
		.nfields = ARRAY_SIZE(gfs2_rgrp_fields),
		.size = sizeof(struct gfs2_rgrp),
	},
	[LGFS2_MT_GFS_RGRP] = {
		.versions = LGFS2_MD_GFS1,
		.header = 1,
		.mh_type = GFS2_METATYPE_RG,
		.mh_format = GFS2_FORMAT_RG,
		.name = "gfs_rgrp",
		.display = "resource group",
		.fields = gfs_rgrp_fields,
		.nfields = ARRAY_SIZE(gfs_rgrp_fields),
		.size = sizeof(struct gfs_rgrp),
	},
	[LGFS2_MT_RGRP_BITMAP] = {
		.versions = LGFS2_MD_GFS1 | LGFS2_MD_GFS2,
		.header = 1,
		.mh_type = GFS2_METATYPE_RB,
		.mh_format = GFS2_FORMAT_RB,
		.name = "gfs2_rgrp_bitmap",
		.display = "allocation bitmap",
		.fields = gfs2_rgrp_bitmap_fields,
		.nfields = ARRAY_SIZE(gfs2_rgrp_bitmap_fields),
		.size = sizeof(struct gfs2_meta_header),
	},
	[LGFS2_MT_GFS2_DINODE] = {
		.versions = LGFS2_MD_GFS2,
		.header = 1,
		.mh_type = GFS2_METATYPE_DI,
		.mh_format = GFS2_FORMAT_DI,
		.name = "gfs2_dinode",
		.display = "inode",
		.fields = gfs2_dinode_fields,
		.nfields = ARRAY_SIZE(gfs2_dinode_fields),
		.size = sizeof(struct gfs2_dinode),
	},
	[LGFS2_MT_GFS_DINODE] = {
		.versions = LGFS2_MD_GFS1,
		.header = 1,
		.mh_type = GFS2_METATYPE_DI,
		.mh_format = GFS2_FORMAT_DI,
		.name = "gfs_dinode",
		.display = "inode",
		.fields = gfs_dinode_fields,
		.nfields = ARRAY_SIZE(gfs_dinode_fields),
		.size = sizeof(struct gfs_dinode),
	},
	[LGFS2_MT_GFS2_INDIRECT] = {
		.versions = LGFS2_MD_GFS2,
		.header = 1,
		.mh_type = GFS2_METATYPE_IN,
		.mh_format = GFS2_FORMAT_IN,
		.name = "gfs2_indirect",
		.display = "indirect",
		.fields = gfs2_indirect_fields,
		.nfields = ARRAY_SIZE(gfs2_indirect_fields),
		.size = sizeof(struct gfs2_meta_header),
	},
	[LGFS2_MT_GFS_INDIRECT] = {
		.versions = LGFS2_MD_GFS1,
		.header = 1,
		.mh_type = GFS2_METATYPE_IN,
		.mh_format = GFS2_FORMAT_IN,
		.name = "gfs_indirect",
		.display = "indirect",
		.fields = gfs_indirect_fields,
		.nfields = ARRAY_SIZE(gfs_indirect_fields),
		.size = sizeof(struct gfs_indirect),
	},
	[LGFS2_MT_DIR_LEAF] = {
		.versions = LGFS2_MD_GFS1 | LGFS2_MD_GFS2,
		.header = 1,
		.mh_type = GFS2_METATYPE_LF,
		.mh_format = GFS2_FORMAT_LF,
		.name = "gfs2_leaf",
		.display = "leaf",
		.fields = gfs2_leaf_fields,
		.nfields = ARRAY_SIZE(gfs2_leaf_fields),
		.size = sizeof(struct gfs2_leaf),
	},
	[LGFS2_MT_JRNL_DATA] = {
		.versions = LGFS2_MD_GFS1 | LGFS2_MD_GFS2,
		.header = 1,
		.mh_type = GFS2_METATYPE_JD,
		.mh_format = GFS2_FORMAT_JD,
		.name = "gfs2_jdata",
		.display = "journal data",
		.fields = gfs2_jdata_fields,
		.nfields = ARRAY_SIZE(gfs2_jdata_fields),
		.size = sizeof(struct gfs2_meta_header),
	},
	[LGFS2_MT_GFS2_LOG_HEADER] = {
		.versions = LGFS2_MD_GFS2,
		.header = 1,
		.mh_type = GFS2_METATYPE_LH,
		.mh_format = GFS2_FORMAT_LH,
		.name = "gfs2_log_header",
		.display = "log header",
		.fields = gfs2_log_header_fields,
		.nfields = ARRAY_SIZE(gfs2_log_header_fields),
		.size = sizeof(struct gfs2_log_header),
	},
	[LGFS2_MT_GFS_LOG_HEADER] = {
		.versions = LGFS2_MD_GFS1,
		.header = 1,
		.mh_type = GFS2_METATYPE_LH,
		.mh_format = GFS2_FORMAT_LH,
		.name = "gfs_log_header",
		.display = "log header",
		.fields = gfs_log_header_fields,
		.nfields = ARRAY_SIZE(gfs_log_header_fields),
		.size = sizeof(struct gfs_log_header),
	},
	[LGFS2_MT_GFS2_LOG_DESC] = {
		.versions = LGFS2_MD_GFS2,
		.header = 1,
		.mh_type = GFS2_METATYPE_LD,
		.mh_format = GFS2_FORMAT_LD,
		.name = "gfs2_log_desc",
		.display = "log descriptor",
		.fields = gfs2_log_desc_fields,
		.nfields = ARRAY_SIZE(gfs2_log_desc_fields),
		.size = sizeof(struct gfs2_log_descriptor),
	},
	[LGFS2_MT_GFS_LOG_DESC] = {
		.versions = LGFS2_MD_GFS1,
		.header = 1,
		.mh_type = GFS2_METATYPE_LD,
		.mh_format = GFS2_FORMAT_LD,
		.name = "gfs_log_desc",
		.display = "log descriptor",
		.fields = gfs_log_desc_fields,
		.nfields = ARRAY_SIZE(gfs_log_desc_fields),
		.size = sizeof(struct gfs_log_descriptor),
	},
	[LGFS2_MT_GFS2_LOG_BLOCK] = {
		.versions = LGFS2_MD_GFS2,
		.header = 1,
		.mh_type = GFS2_METATYPE_LB,
		.mh_format = GFS2_FORMAT_LB,
		.name = "gfs2_log_block",
		.display = "log",
		.fields = gfs2_log_block_fields,
		.nfields = ARRAY_SIZE(gfs2_log_block_fields),
		.size = sizeof(struct gfs2_meta_header),
	},
	[LGFS2_MT_EA_ATTR] = {
		.versions = LGFS2_MD_GFS1 | LGFS2_MD_GFS2,
		.header = 1,
		.mh_type = GFS2_METATYPE_EA,
		.mh_format = GFS2_FORMAT_EA,
		.name = "gfs2_ea_attr",
		.display = "extended attribute",
		.fields = gfs2_ea_attr_fields,
		.nfields = ARRAY_SIZE(gfs2_ea_attr_fields),
		.size = sizeof(struct gfs2_meta_header),
	},
	[LGFS2_MT_EA_DATA] = {
		.versions = LGFS2_MD_GFS1 | LGFS2_MD_GFS2,
		.header = 1,
		.mh_type = GFS2_METATYPE_ED,
		.mh_format = GFS2_FORMAT_ED,
		.name = "gfs2_ea_data",
		.display = "extended attribute data",
		.fields = gfs2_ea_data_fields,
		.nfields = ARRAY_SIZE(gfs2_ea_data_fields),
		.size = sizeof(struct gfs2_meta_header),
	},
	[LGFS2_MT_GFS2_QUOTA_CHANGE] = {
		.versions = LGFS2_MD_GFS2,
		.name = "gfs2_quota_change",
		.display = "quota change",
		.fields = gfs2_quota_change_fields,
		.nfields = ARRAY_SIZE(gfs2_quota_change_fields),
		.size = sizeof(struct gfs2_quota_change),
	},
	[LGFS2_MT_DIRENT] = {
		.versions = LGFS2_MD_GFS1 | LGFS2_MD_GFS2,
		.name = "gfs2_dirent",
		.display = "directory entry",
		.fields = gfs2_dirent_fields,
		.nfields = ARRAY_SIZE(gfs2_dirent_fields),
		.size = sizeof(struct gfs2_dirent),
	},
	[LGFS2_MT_EA_HEADER] = {
		.versions = LGFS2_MD_GFS1 | LGFS2_MD_GFS2,
		.name = "gfs2_ea_header",
		.display = "extended attribute header",
		.fields = gfs2_ea_header_fields,
		.nfields = ARRAY_SIZE(gfs2_ea_header_fields),
		.size = sizeof(struct gfs2_ea_header),
	},
	[LGFS2_MT_GFS2_INUM_RANGE] = {
		.versions = LGFS2_MD_GFS2,
		.name = "gfs2_inum_range",
		.display = "inum range",
		.fields = gfs2_inum_range_fields,
		.nfields = ARRAY_SIZE(gfs2_inum_range_fields),
		.size = sizeof(struct gfs2_inum_range),
	},
	[LGFS2_MT_STATFS_CHANGE] = {
		.versions = LGFS2_MD_GFS1 | LGFS2_MD_GFS2,
		.name = "gfs2_statfs_change",
		.display = "statfs change",
		.fields = gfs2_statfs_change_fields,
		.nfields = ARRAY_SIZE(gfs2_statfs_change_fields),
		.size = sizeof(struct gfs2_statfs_change),
	},
	[LGFS2_MT_GFS_JINDEX] = {
		.versions = LGFS2_MD_GFS1,
		.name = "gfs_jindex",
		.display = "journal index",
		.fields = gfs_jindex_fields,
		.nfields = ARRAY_SIZE(gfs_jindex_fields),
		.size = sizeof(struct gfs_jindex),
	},
	[LGFS2_MT_GFS_BLOCK_TAG] = {
		.versions = LGFS2_MD_GFS1,
		.name = "gfs_block_tag",
		.display = "block tag",
		.fields = gfs_block_tag_fields,
		.nfields = ARRAY_SIZE(gfs_block_tag_fields),
		.size = sizeof(struct gfs_block_tag),
	},
	[LGFS2_MT_DATA] = {
		.versions = LGFS2_MD_GFS1 | LGFS2_MD_GFS2,
		.name = "data",
		.display = "data",
	},
	[LGFS2_MT_FREE] = {
		.versions = LGFS2_MD_GFS1 | LGFS2_MD_GFS2,
		.name = "free",
		.display = "free",
	},
};

const unsigned lgfs2_metadata_size = ARRAY_SIZE(lgfs2_metadata);

const struct lgfs2_metafield *lgfs2_find_mfield_name(const char *name, const struct lgfs2_metadata *mtype)
{
	int j;
	const struct lgfs2_metafield *f;

	for (j = 0; j < mtype->nfields; j++) {
		f = &mtype->fields[j];
		if (strcmp(f->name, name) == 0)
			return f;
	}
	return NULL;
}

const struct lgfs2_metadata *lgfs2_find_mtype(uint32_t mh_type, const unsigned versions)
{
	const struct lgfs2_metadata *m = lgfs2_metadata;
	unsigned n = 0;

	do {
		if ((m[n].versions & versions) && m[n].mh_type == mh_type)
			return &m[n];
		n++;
	} while (n < lgfs2_metadata_size);

	return NULL;
}

const struct lgfs2_metadata *lgfs2_find_mtype_name(const char *name, const unsigned versions)
{
	const struct lgfs2_metadata *m = lgfs2_metadata;
	unsigned n = 0;

	do {
		if ((m[n].versions & versions) && !strcmp(m[n].name, name))
			return &m[n];
		n++;
	} while (n < lgfs2_metadata_size);

	return NULL;
}

int lgfs2_field_str(char *str, const size_t size, const char *blk, const struct lgfs2_metafield *field, int hex)
{
	const char *fieldp = blk + field->offset;

	errno = EINVAL;
	if (str == NULL)
		return 1;

	if (field->flags & LGFS2_MFF_UUID) {
		char readable_uuid[36+1];
		uuid_t uuid;

		memcpy(uuid, fieldp, sizeof(uuid_t));
		uuid_unparse(uuid, readable_uuid);
		snprintf(str, size, "%s", readable_uuid);
	} else if (field->flags & LGFS2_MFF_STRING) {
		snprintf(str, size, "%s", fieldp);
	} else {
		uint64_t value;

		switch(field->length) {
		case sizeof(uint8_t):
			value = *(uint8_t *)fieldp;
			break;
		case sizeof(uint16_t):
			value = be16_to_cpu(*(__be16 *)fieldp);
			break;
		case sizeof(uint32_t):
			value = be32_to_cpu(*(__be32 *)fieldp);
			break;
		case sizeof(uint64_t):
			value = be64_to_cpu(*(__be64 *)fieldp);
			break;
		default:
			*str = 0;
			goto out;
		}

		if (hex) {
			snprintf(str, size, "%"PRIx64, value);
			goto out;
		}
		if (field->flags & LGFS2_MFF_MASK) {
			char *s = str, *end = str + size;
			unsigned n;

			for (n = 0; n < field->nsyms; n++) {
				const struct lgfs2_symbolic *sym = &field->symtab[n];

				if (!(value & sym->key))
					continue;
				if (s != str && s < end)
					*s++ = '/';
				if (s < end) {
					const char *v = sym->value + sym->prefix;
					size_t len = strlen(v);

					if (len > end - s)
						len = end - s;
					memcpy(s, v, len);
					s += len;
				}
			}
			if (s < end)
				*s = '\0';
			goto out;
		}
		snprintf(str, size, "%"PRIu64, value);
	}
out:
	str[size - 1] = '\0';
	return 0;
}

int lgfs2_field_assign(char *blk, const struct lgfs2_metafield *field, const void *val)
{
	char *fieldp = blk + field->offset;
	uint64_t num = *(uint64_t *)val;

	if (field->flags & LGFS2_MFF_UUID) {
		memcpy(fieldp, val, 16);
		return 0;
	}

	errno = EINVAL;
	if (field->flags & LGFS2_MFF_STRING) {
		size_t len = strnlen(val, field->length);

		if (len >= field->length)
			return 1;
		strncpy(fieldp, val, field->length - 1);
		fieldp[field->length - 1] = '\0';
		return 0;
	}

	switch(field->length) {
	case sizeof(uint8_t):
		*fieldp = (uint8_t)num;
		return 0;
	case sizeof(uint16_t):
		*(__be16 *)fieldp = cpu_to_be16((uint16_t)num);
		return 0;
	case sizeof(uint32_t):
		*(__be32 *)fieldp = cpu_to_be32((uint32_t)num);
		return 0;
	case sizeof(uint64_t):
		*(__be64 *)fieldp = cpu_to_be64((uint64_t)num);
		return 0;
	default:
		/* Will never happen */
		break;
	}

	return 1;
}
