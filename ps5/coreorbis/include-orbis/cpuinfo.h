#pragma once
// Orbis cpuinfo shim: fixed console topology (PS5 Pro Zen2 8C/16T; PS4 Jaguar 8C/8T, no SMT).
// Replaces 3rdparty/cpuinfo lib, which needs OS arch-detection missing on Orbis.
// Feature queries match hardware (poc1 CPUID: SSE4.1+AVX+AVX2+FMA+BMI, no AVX512; Jaguar: no AVX2/FMA3/BMI2).
#include <stdint.h>

#define CPUINFO_ABI

#ifdef __cplusplus
extern "C" {
#endif

enum cpuinfo_vendor
{
  cpuinfo_vendor_unknown = 0,
  cpuinfo_vendor_intel = 1,
  cpuinfo_vendor_amd = 2
};

enum cpuinfo_uarch
{
  cpuinfo_uarch_unknown = 0,
  cpuinfo_uarch_haswell = 0x00100208,
  cpuinfo_uarch_zen2 = 0x0020010A,
  cpuinfo_uarch_jaguar = 0x00200202
};

// Compile-time topology facts for the fixed Orbis shim. Jaguar is two four-core modules with one thread per
// core; each core has 32 KB L1i and 32 KB L1d with 64-byte lines, and each module shares a 2 MB 64-byte-line L2.
// The implementation is in orbis-shims/cpuinfo_stubs.cpp; these constants are the single source it should use.
#ifdef ORBIS_PS4
#define ORBIS_CPUINFO_THREADS_PER_CORE 1u
#define ORBIS_CPUINFO_CORE_COUNT 8u
#define ORBIS_CPUINFO_PROCESSOR_COUNT 8u
#define ORBIS_CPUINFO_CLUSTER_COUNT 2u
#define ORBIS_CPUINFO_CLUSTER_CORE_COUNT 4u
#define ORBIS_CPUINFO_UARCH cpuinfo_uarch_jaguar
#define ORBIS_CPUINFO_L2_SIZE (2u * 1024u * 1024u)
#define ORBIS_CPUINFO_L2_PROCESSOR_COUNT 4u
#else
#define ORBIS_CPUINFO_THREADS_PER_CORE 2u
#define ORBIS_CPUINFO_CORE_COUNT 8u
#define ORBIS_CPUINFO_PROCESSOR_COUNT 16u
#define ORBIS_CPUINFO_CLUSTER_COUNT 1u
#define ORBIS_CPUINFO_CLUSTER_CORE_COUNT 8u
#define ORBIS_CPUINFO_UARCH cpuinfo_uarch_zen2
#define ORBIS_CPUINFO_L2_SIZE (512u * 1024u)
#define ORBIS_CPUINFO_L2_PROCESSOR_COUNT 2u
#endif
#define ORBIS_CPUINFO_L1I_SIZE (32u * 1024u)
#define ORBIS_CPUINFO_L1D_SIZE (32u * 1024u)
#define ORBIS_CPUINFO_CACHE_LINE_SIZE 64u

struct cpuinfo_cache
{
  // line_size stays first because cpuinfo_stubs.cpp uses positional initializers. The omitted fields get their
  // compile-time defaults there; the fixed cache descriptors below carry the L1/L2 sizes, lines and sharing counts.
  uint32_t line_size;
#ifdef __cplusplus
  uint32_t size = ORBIS_CPUINFO_L1D_SIZE;
  uint32_t processor_count = 1u;
#else
  uint32_t size;
  uint32_t processor_count;
#endif
};

#ifdef __cplusplus
inline const cpuinfo_cache cpuinfo_orbis_l1i = {ORBIS_CPUINFO_CACHE_LINE_SIZE, ORBIS_CPUINFO_L1I_SIZE, 1u};
inline const cpuinfo_cache cpuinfo_orbis_l1d = {ORBIS_CPUINFO_CACHE_LINE_SIZE, ORBIS_CPUINFO_L1D_SIZE, 1u};
inline const cpuinfo_cache cpuinfo_orbis_l2 = {
  ORBIS_CPUINFO_CACHE_LINE_SIZE, ORBIS_CPUINFO_L2_SIZE, ORBIS_CPUINFO_L2_PROCESSOR_COUNT};
#endif

struct cpuinfo_cluster
{
  uint32_t processor_start;
  uint32_t processor_count;
  uint32_t core_start;
  uint32_t core_count;
  uint32_t cluster_id;
};

struct cpuinfo_core
{
  uint32_t processor_start;
  uint32_t processor_count;
  uint32_t core_id;
  const struct cpuinfo_cluster* cluster;
  enum cpuinfo_vendor vendor;
};

struct cpuinfo_uarch_info
{
  enum cpuinfo_uarch uarch;
#ifdef __cplusplus
  // PS4 port (AI-assisted): cpuinfo_stubs.cpp still initializes this with the PS5's Zen2 constant. The shim is
  // fixed-hardware, so keep the constructor's result native to the build until that file can use
  // ORBIS_CPUINFO_UARCH directly. Needs proper testing.
  constexpr cpuinfo_uarch_info() : uarch(ORBIS_CPUINFO_UARCH) {}
#ifdef ORBIS_PS4
  constexpr cpuinfo_uarch_info(enum cpuinfo_uarch) : uarch(ORBIS_CPUINFO_UARCH) {}
#else
  constexpr cpuinfo_uarch_info(enum cpuinfo_uarch value) : uarch(value) {}
#endif
#endif
};

struct cpuinfo_processor_cache
{
#ifdef __cplusplus
  const struct cpuinfo_cache* l1i = &cpuinfo_orbis_l1i;
  const struct cpuinfo_cache* l1d = &cpuinfo_orbis_l1d;
  const struct cpuinfo_cache* l2 = &cpuinfo_orbis_l2;
  const struct cpuinfo_cache* l3 = nullptr;
#else
  const struct cpuinfo_cache* l1i;
  const struct cpuinfo_cache* l1d;
  const struct cpuinfo_cache* l2;
  const struct cpuinfo_cache* l3;
#endif
};

struct cpuinfo_processor
{
  uint32_t smt_id;
  const struct cpuinfo_cluster* cluster;
  struct cpuinfo_processor_cache cache;
};

bool cpuinfo_initialize(void);
const struct cpuinfo_processor* cpuinfo_get_processor(uint32_t index);
uint32_t cpuinfo_get_processors_count(void);
uint32_t cpuinfo_get_clusters_count(void);
const struct cpuinfo_cluster* cpuinfo_get_cluster(uint32_t index);
uint32_t cpuinfo_get_cores_count(void);
const struct cpuinfo_core* cpuinfo_get_core(uint32_t index);
uint32_t cpuinfo_get_uarchs_count(void);
const struct cpuinfo_uarch_info* cpuinfo_get_uarch(uint32_t index);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
// Feature queries used by VMManager/MultiISA startup checks.
static inline bool cpuinfo_has_x86_sse4_1(void)
{
  return true;
}
static inline bool cpuinfo_has_x86_avx(void)
{
  return true;
}
// PS4 port (AI-assisted): the PS4's (and PS4 Pro's) AMD Jaguar has SSE4.2, AVX, BMI1 and F16C, but no AVX2, FMA3 or
// BMI2: PCSX2 must not pick AVX2 code (MultiISA, the GS's code generators) there.
#ifdef ORBIS_PS4
#define ORBIS_CPU_HAS_AVX2 false
#else
#define ORBIS_CPU_HAS_AVX2 true
#endif
static inline bool cpuinfo_has_x86_avx2(void)
{
  return ORBIS_CPU_HAS_AVX2;
}
static inline bool cpuinfo_has_x86_avx512f(void)
{
  return false;
}
static inline bool cpuinfo_has_x86_fma3(void)
{
  return ORBIS_CPU_HAS_AVX2; // Zen 2 has FMA3, Jaguar doesn't
}
static inline bool cpuinfo_has_x86_bmi(void)
{
  return true;
}
static inline bool cpuinfo_has_x86_bmi2(void)
{
  return ORBIS_CPU_HAS_AVX2; // Zen 2 has BMI2, Jaguar doesn't
}
#endif
