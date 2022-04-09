#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#if defined(__clang__)
  #define PRAGMA_LOOP_VECTORIZE _Pragma("clang loop vectorize(enable)")
#elif defined(__GNUC__)
  #define PRAGMA_LOOP_VECTORIZE _Pragma("GCC ivdep")
#else
  #define PRAGMA_LOOP_VECTORIZE _Pragma("ivdep")
#endif

#define WF_NEG_INF (-0x40000000)

static void wf_pad_str(int32_t tl, const char *ts, int32_t ql, const char *qs, char **pts, char **pqs)
{
	uint8_t t[256];
	int32_t i, c1 = -1, c2 = -1;
	char *s1, *s2;
	*pts = *pqs = 0;
	// collect all used characters
	memset(t, 0, 256);
	for (i = 0; i < tl; ++i)
		if (t[(uint8_t)ts[i]] == 0)
			t[(uint8_t)ts[i]] = 1;
	for (i = 0; i < ql; ++i)
		if (t[(uint8_t)qs[i]] == 0)
			t[(uint8_t)qs[i]] = 1;
	for (i = 0; i < 256; ++i)
		if (t[i] == 0) {
			if (c1 < 0) c1 = i;
			else if (c2 < 0) c2 = i;
		}
	if (c1 < 0 || c2 < 0) return; // The two strings use >=255 characters. Unlikely for bio strings.
	s1 = (char*)malloc(tl + ql + 16); // the two strings are allocated together
	s2 = s1 + tl + 8;
	memcpy(s1, ts, tl);
	for (i = tl; i < tl + 8; ++i) s1[i] = c1; // pad with c1
	memcpy(s2, qs, ql);
	for (i = ql; i < ql + 8; ++i) s2[i] = c2; // pad with c2
	*pts = s1, *pqs = s2;
}

static inline int32_t wf_extend1_padded(const char *ts, const char *qs, int32_t k, int32_t d)
{
	uint64_t cmp = 0;
	const char *ts_ = ts + 1;
	const char *qs_ = qs + d + 1;
	while (1) {
		uint64_t x = *(uint64_t*)(ts_ + k); // warning: unaligned memory access
		uint64_t y = *(uint64_t*)(qs_ + k);
		cmp = x ^ y;
		if (cmp == 0) k += 8;
		else break;
	}
	k += __builtin_ctzl(cmp) >> 3;
	return k;
}

static inline void wf_prune_global(int32_t tl, int32_t ql, int32_t *lo, int32_t *hi, const int32_t *H)
{
	int32_t d, min = INT32_MAX, l = *lo, h = *hi;
	for (d = l; d <= h; ++d) {
		int32_t i = d + H[d];
		int32_t x = ql - i > tl - H[d]? ql - i : tl - H[d];
		min = min < x? min : x; // min is the max possible distance
	}
	for (d = l; d <= h; ++d) {
		int32_t i = d + H[d];
		int32_t x = (ql - i) - (tl - H[d]); // x is the least possible distance from b[k]
		x = x >= 0? x : -x;
		if (x < min) break;
	}
	l = d;
	for (d = h; d >= l; --d) {
		int32_t i = d + H[d];
		int32_t x = (ql - i) - (tl - H[d]);
		x = x >= 0? x : -x;
		if (x < min) break;
	}
	h = d;
	*lo = l, *hi = h;
}

int32_t u85_4(int32_t tl, const char *ts, int32_t ql, const char *qs)
{
	int32_t *a[2], *H, lo = 0, hi = 0, s = 0;
	char *pts, *pqs;
	wf_pad_str(tl, ts, ql, qs, &pts, &pqs);
	a[0] = (int32_t*)malloc((tl + ql + 5) * 2 * sizeof(int32_t));
	a[1] = a[0] + tl + ql + 5;
	H = a[1] + 2;
	H[0] = -1, H[-2] = H[-1] = H[1] = H[2] = WF_NEG_INF; // pad with WF_NEG_INF
	while (1) {
		int32_t d, *G;
		for (d = lo; d <= hi; ++d) {
			int32_t k = H[d];
			if (k < -1 || d + k < -1 || k >= tl || d + k >= ql) continue;
			k = wf_extend1_padded(pts, pqs, k, d);
			if (k == tl - 1 && d + k == ql - 1) break;
			H[d] = k;
		}
		if (d <= hi) break;
		if ((s&0x1f) == 0) wf_prune_global(tl, ql, &lo, &hi, H);
		if (lo > -tl) --lo;
		if (hi <  ql) ++hi;
		G = a[s&1] + 2 - lo;
		PRAGMA_LOOP_VECTORIZE
		for (d = lo; d <= hi; ++d) {
			int32_t k = H[d-1];
			k = k > H[d]   + 1? k : H[d]   + 1;
			k = k > H[d+1] + 1? k : H[d+1] + 1;
			G[d] = k;
		}
		G[lo-2] = G[lo-1] = G[hi+1] = G[hi+2] = WF_NEG_INF;
		H = G;
		++s;
	}
	free(a[0]);
	free(pts);
	return s;
}
