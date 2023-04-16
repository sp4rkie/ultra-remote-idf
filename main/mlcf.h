
//
// mlcf.h - my little C foundation
//
// derived from:
//   source_edit_V51.190.CVS/fsc-fct4-src/fct4/fct4_hw4.h
//

// --- vvv ------------------------- value added SLI4 target mode ---------------------------------

// ESP8266 & ESP32 version included

typedef unsigned char  _u8;
typedef unsigned short _u16;
typedef unsigned int   _u32;
#if defined(ESP8266) || defined(ESP32) || defined(CONFIG_IDF_TARGET_ESP32)
typedef unsigned long long _u64;
#else
typedef unsigned long _u64;
#endif
typedef _u8           *_u8p;
typedef _u16          *_u16p;
typedef _u32          *_u32p;
typedef _u64          *_u64p;

typedef char           _i8;
typedef short          _i16;
typedef int            _i32;
#if defined(ESP8266) || defined(ESP32) || defined(CONFIG_IDF_TARGET_ESP32)
typedef long long      _i64;
#else
typedef long           _i64;
#endif
typedef _i8           *_i8p;
typedef _i16          *_i16p;
typedef _i32          *_i32p;
typedef _i64          *_i64p;

#define _SZ(a) (sizeof a)
#define _NE(a) (_SZ(a) / _SZ(*(a))) // number of elements

//
// a neat debug foundation [ see fct4_glue.c how to use ]
//

// --- vvv --- debug section --- vvv ---
// debug's main purpose is to record driver internal
// code paths and data processing. to gain some information
// that is not always obvious.

//#define PR05_(fmt, a...) printk("[ %02d ] " fmt, smp_processor_id(), ##a)              // sel1: activate debug to dmesg
//#define PR05_(fmt, a...) trc_m(fct4_debugid, "[ %02d ] " fmt, smp_processor_id(), ##a) // sel1: activate debug to buffer
#define PR04_(fmt, a...) fprintf(stderr, fmt, ##a)
#define PR04(a...) PR04_(a);
#if defined(ESP8266) || defined(ESP32)
#define PR05_(fmt, a...) Serial.printf(fmt, ##a)
#else
#define PR05_(fmt, a...) printf(fmt, ##a)
#endif
#define PR05(a...) PR05_(a);
//#define PR06_(a...) PR05_(a)                                                           // sel2: activate bf_xxx debug
#define PR06(a...)                                                                     // not used
#define PR07(a...) printk(a);                                                          // permanent log
#define TP04 PR04_("%s\n", __func__);
#define TP05 PR05_("%s\n", __func__);
#define TP07 PR07("%s\n", __func__);
#define GV05(a) PR05_("val: %s == 0x%x\n", #a, (_u32)(a));
#define GW05(a) PR05_("val: %s == 0x%llx\n", #a, (_u64)(a));
#define GV07(a) PR07("val: %s == 0x%x\n", #a, (_u32)(a));
#define GS05(a) PR05_("str: %s [ %s ]\n", #a, a);
#define GS07(a) PR07("str: %s [ %s ]\n", #a, a);
#define DM05(a, b, c) PR05("raw: " #a " "); fct4_dump_u32((_u8p)(a), (b), (c));
#define GI05(a) b == (a) ? #a :
#define GJ05(a) if (b & (a)) len += snprintf(buf + len, sizeof buf - len, #a " ");
#define DI05(a) // fct4_dump_iocb(a, __func__, __FILE__, __LINE__, #a); // key structure sampled at some strategic places
#define DW05(a) // fct4_dump_wqe(a, __func__, __FILE__, __LINE__, #a);  // key structure sampled at some strategic places
#define DS05(a) // fct4_dump_sif(a, __func__, __FILE__, __LINE__, #a);  // key structure sampled at some strategic places
// --- ^^^ --- debug section --- ^^^ ---

// --- vvv --- trace section --- vvv ---
// trace's main purpose is to record communication between
// the board and components externally connected. this
// may provide some information essential to discover 
// problems on both host and target side.

//#define PR15_(fmt, a...) printk("[ %02d ] " fmt, smp_processor_id(), ##a)              // sel3: activate tracing to dmesg
#define PR15_(fmt, a...) trc_m(fct4_traceid, "[ %02d ] " fmt, smp_processor_id(), ##a) // sel3: activate tracing to buffer
#define PR15(a...) PR15_(a);
// --- ^^^ --- trace section --- ^^^ ---

#define SWP_16(a) \
    ((_u32)(a) << 8 & 0xff00 | \
     (_u32)(a) >> 8 & 0x00ff)

#define SWP_24(a) \
    ((a)[0] << 16 | \
     (a)[1] <<  8 | \
     (a)[2] <<  0)

#define SWP_32(a) \
    ((_u32)(a) << 24 & 0xff000000 | \
     (_u32)(a) <<  8 & 0x00ff0000 | \
     (_u32)(a) >>  8 & 0x0000ff00 | \
     (_u32)(a) >> 24 & 0x000000ff)

#define SWP_64(a) \
    ((_u64)(a) << 56 & 0xff00000000000000 | \
     (_u64)(a) << 40 & 0x00ff000000000000 | \
     (_u64)(a) << 24 & 0x0000ff0000000000 | \
     (_u64)(a) <<  8 & 0x000000ff00000000 | \
     (_u64)(a) >>  8 & 0x00000000ff000000 | \
     (_u64)(a) >> 24 & 0x0000000000ff0000 | \
     (_u64)(a) >> 40 & 0x000000000000ff00 | \
     (_u64)(a) >> 56 & 0x00000000000000ff)

#define fct4_printf_vlog(vport, level, mask, fmt, arg...) \
do { \
        { if (((mask) & (vport)->cfg_log_verbose) || (level[1] <= '3')) \
                PR15("fct4.%d:(%d): " fmt, (vport)->phba->brd_no, vport->vpi, ##arg); } \
} while (0)

#define fct4_printf_log(phba, level, mask, fmt, arg...) \
do { \
        { uint32_t log_verbose = (phba)->pport ? \
                                 (phba)->pport->cfg_log_verbose : \
                                 (phba)->cfg_log_verbose; \
          if (((mask) & log_verbose) || (level[1] <= '3')) \
                PR15("fct4.%d: " fmt, phba->brd_no, ##arg); \
        } \
} while (0)

#define fct4_debugfs_disc_trc(a, b, c, d...) PR15_(c "\n", d)
#define fct4_debugfs_disc_trc_s(a, b, c, d...) printk(c "\n", d)
#define fct4_debugfs_disc_trc5_1(a, b, c, d, e) fct4_debugfs_disc_trc(a, b, c, d)
#define fct4_debugfs_disc_trc6_1(a, b, c, d, e, f) fct4_debugfs_disc_trc(a, b, c, d, e)
#define fct4_debugfs_disc_trc6_2(a, b, c, d, e, f) fct4_debugfs_disc_trc(a, b, c, d)

#define fct4_debugfs_slow_ring_trc(a, b, c...) PR15_(b "\n", c)
#define fct4_debugfs_slow_ring_trc5_1(a, b, c, d, e) fct4_debugfs_slow_ring_trc(a, b, c, d)
#define fct4_debugfs_slow_ring_trc5_2(a, b, c, d, e) fct4_debugfs_slow_ring_trc(a, b, c)

#ifdef PR05_
#define writel_(a, b) (PR05_("writel: *" #b " = " #a " [ *0x%016lx = 0x%lx ]\n", (_u64)(b), (_u64)(a)), writel((a), (b)))
#define readl_(a) (PR05_("readl: *" #a " [ *0x%016lx == 0x%lx ]\n", (_u64)(a), (_u64)readl(a)), readl(a))

#define writel(a...) writel_(a)
#define readl(a) readl_(a)
#else
#define PR05_(a...)
#endif

#define FCT4_IS_TARGET_CMD(cmd) \
    ((cmd) == CMD_FCP_TSEND64_CX \
  || (cmd) == CMD_FCP_TRECEIVE64_CX \
  || (cmd) == CMD_FCP_TRSP64_CX)

#define v___MODE_FCP_TARGET______v if (fct4_target_mode) {
#define v___MODE_FCP_INITIATOR___v } else {
#define v___MODE_SB2_FICON_______v x
#define o___MODE_________________o }

// each time they issue a new lpfc version the data format of phba->sli4_hba.link_state.speed changes
// --> try to neutralize the impact with a central wrapper
#define FCT4_UNCRAPITIZE_LINK_SPEED(a) ((a) > 1000 ? (a) / 1000 : (a))

// --- ^^^ ------------------------- value added SLI4 target mode ---------------------------------


/* Macros to deal with bit fields. Each bit field must have 3 #defines
 * associated with it (_SHIFT, _MASK, and _WORD).
 * EG. For a bit field that is in the 7th bit of the "field4" field of a
 * structure and is 2 bits in size the following #defines must exist:
 *      struct temp {
 *              uint32_t        field1;
 *              uint32_t        field2;
 *              uint32_t        field3;
 *              uint32_t        field4;
 *      #define example_bit_field_SHIFT         7
 *      #define example_bit_field_MASK          0x03
 *      #define example_bit_field_WORD          field4
 *              uint32_t        field5;
 *      };
 * Then the macros below may be used to get or set the value of that field.
 * EG. To get the value of the bit field from the above example:
 *      struct temp t1;
 *      value = bf_get(example_bit_field, &t1);
 * And then to set that bit field:
 *      bf_set(example_bit_field, &t1, 2);
 * Or clear that bit field:
 *      bf_set(example_bit_field, &t1, 0);
 */
#define bf_get_be32(name, ptr) \
        ((be32_to_cpu((ptr)->name##_WORD) >> name##_SHIFT) & name##_MASK)
#define bf_get_le32(name, ptr) \
        ((le32_to_cpu((ptr)->name##_WORD) >> name##_SHIFT) & name##_MASK)
#define bf_get(name, ptr) \
        (((ptr)->name##_WORD >> name##_SHIFT) & name##_MASK)
#define bf_set_le32(name, ptr, value) \
        ((ptr)->name##_WORD = cpu_to_le32(((((value) & \
        name##_MASK) << name##_SHIFT) | (le32_to_cpu((ptr)->name##_WORD) & \
        ~(name##_MASK << name##_SHIFT)))))
#define bf_set(name, ptr, value) \
        ((ptr)->name##_WORD = ((((value) & name##_MASK) << name##_SHIFT) | \
                 ((ptr)->name##_WORD & ~(name##_MASK << name##_SHIFT))))

#ifdef PR06_
#define bf_get_be32_(name, ptr) \
        (PR06_("bf_get_be32: *" #ptr "->" #name " [ *0x%016lx == 0x%08x ]\n", \
                (_u64)&(ptr)->name##_WORD, bf_get_be32_(name, ptr)), \
                \
                bf_get_be32(name, ptr))
#define bf_get_le32_(name, ptr) \
        (PR06_("bf_get_le32: *" #ptr "->" #name " [ *0x%016lx == 0x%08x ]\n", \
                (_u64)&(ptr)->name##_WORD, bf_get_le32_(name, ptr)), \
                \
                bf_get_le32(name, ptr))
#define bf_get_(name, ptr) \
        (PR06_("bf_get: *" #ptr "->" #name " [ *0x%016lx == 0x%08x ]\n", \
                (_u64)&(ptr)->name##_WORD, bf_get_(name, ptr)), \
                \
                bf_get(name, ptr))
#define bf_set_le32_(name, ptr, value) \
        (PR06_("bf_set_le32: *" #ptr "->" #name " <- " #value " [ *0x%016lx <- (0x%lx & 0x%x) << %d ]\n", \
                (_u64)&(ptr)->name##_WORD, (_u64)(value), name##_MASK, name##_SHIFT), \
                \
                bf_set_le32(name, ptr, value))
#define bf_set_(name, ptr, value) \
        (PR06_("bf_set: *" #ptr "->" #name " <- " #value " [ *0x%016lx <- (0x%lx & 0x%x) << %d ]\n", \
                (_u64)&(ptr)->name##_WORD, (_u64)(value), name##_MASK, name##_SHIFT), \
                \
                bf_set(name, ptr, value))

#define bf_get_be32 bf_get_be32_
#define bf_get_le32 bf_get_le32_
#define bf_get bf_get_
#define bf_set_le32 bf_set_le32_
#define bf_set bf_set_
#else
#define PR06_(a...)
#endif

