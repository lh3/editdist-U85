#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

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
	const uint64_t *ts_ = (const uint64_t*)(ts + k + 1);
	const uint64_t *qs_ = (const uint64_t*)(qs + d + k + 1);
	uint64_t cmp = *ts_ ^ *qs_;
	while (cmp == (uint64_t)0) {
		++ts_, ++qs_, k += 8;
		cmp = *ts_ ^ *qs_;
	}
	k += __builtin_ctzl(cmp) >> 3;
	return k;
}

typedef struct {
	int32_t lo, hi;
	uint64_t *a;
} wf_tb1_t;

typedef struct {
	int32_t m, n;
	wf_tb1_t *a;
} wf_tb_t;

static uint64_t *wf_tb_add(wf_tb_t *tb, int32_t lo, int32_t hi)
{
	wf_tb1_t *p;
	if (tb->n == tb->m) {
		tb->m = tb->m + (tb->m>>1) + 4;
		tb->a = (wf_tb1_t*)realloc(tb->a, tb->m * sizeof(*tb->a));
	}
	p = &tb->a[tb->n++];
	p->lo = lo, p->hi = hi;
	p->a = (uint64_t*)calloc((hi - lo + 1 + 31) / 32, sizeof(uint64_t));
	return p->a;
}

typedef struct {
	int32_t m, n;
	uint32_t *cigar;
} wf_cigar_t;

static void wf_cigar_push1(wf_cigar_t *c, int32_t op, int32_t len)
{
	if (c->n && op == (c->cigar[c->n-1]&0xf)) {
		c->cigar[c->n-1] += len<<4;
	} else {
		if (c->n == c->m) {
			c->m = c->m + (c->m>>1) + 4;
			c->cigar = (uint32_t*)realloc(c->cigar, c->m * sizeof(*c->cigar));
		}
		c->cigar[c->n++] = len<<4 | op;
	}
}

static uint32_t *wf_traceback(int32_t t_end, const char *ts, int32_t q_end, const char *qs, wf_tb_t *tb, int32_t *n_cigar)
{
	wf_cigar_t cigar = {0,0,0};
	int32_t i = q_end, k = t_end, s = tb->n - 1;
	for (;;) {
		int32_t k0 = k, j, pre;
		while (i >= 0 && k >= 0 && qs[i] == ts[k])
			--i, --k;
		if (k0 - k > 0)	
			wf_cigar_push1(&cigar, 7, k0 - k);
		if (i < 0 || k < 0) break;
		assert(s >= 0);
		j = i - k - tb->a[s].lo;
		assert(j <= tb->a[s].hi - tb->a[s].lo);
		pre = tb->a[s].a[j>>5] >> (j&0x1f)*2 & 0x3;
		if (pre == 1) {
			wf_cigar_push1(&cigar, 8, 1);
			--i, --k;
		} else if (pre == 0) {
			wf_cigar_push1(&cigar, 1, 1);
			--i;
		} else {
			wf_cigar_push1(&cigar, 2, 1);
			--k;
		}
		--s;
	}
	if (i >= 0) wf_cigar_push1(&cigar, 1, i + 1);
	else if (k >= 0) wf_cigar_push1(&cigar, 2, k + 1);
	for (i = 0; i < cigar.n>>1; ++i) {
		uint32_t t = cigar.cigar[i];
		cigar.cigar[i] = cigar.cigar[cigar.n - i - 1];
		cigar.cigar[cigar.n - i - 1] = t;
	}
	*n_cigar = cigar.n;
	return cigar.cigar;
}

uint32_t *u85_cigar(int32_t tl, const char *ts, int32_t ql, const char *qs, int32_t *ed, int32_t *nc)
{
	int32_t *a[2], *H, lo = 0, hi = 0, s = 0, i;
	char *pts, *pqs;
	uint8_t *x0 = 0;
	uint32_t *cigar = 0;
	wf_tb_t tb = {0,0,0};

	wf_pad_str(tl, ts, ql, qs, &pts, &pqs);
	a[0] = (int32_t*)malloc((tl + ql + 5) * 2 * sizeof(int32_t) + (tl + ql + 5));
	a[1] = a[0] + tl + ql + 5;
	x0 = (uint8_t*)(a[1] + tl + ql + 5);
	H = a[1] + 2;
	H[0] = -1, H[-2] = H[-1] = H[1] = H[2] = WF_NEG_INF; // pad with WF_NEG_INF

	while (1) {
		int32_t d, *G;
		for (d = lo; d <= hi; ++d) {
			int32_t k = H[d];
			k = wf_extend1_padded(pts, pqs, k, d);
			if (k == tl - 1 && d + k == ql - 1) break;
			H[d] = k;
		}
		if (d <= hi) break;
		if (((s+1)&0x1f) == 0) { // shrink the band every 32 cycles
			int32_t min = INT32_MAX;
			for (d = lo; d <= hi; ++d) {
				int32_t k = H[d], i = d + k;
				int32_t x = ql - i > tl - k? ql - i : tl - k;
				min = min < x? min : x;
			}
			while ((ql - tl) - lo >= min) ++lo;
			while (hi - (ql - tl) >= min) --hi;
		}
		if (lo > -tl) --lo;
		if (hi <  ql) ++hi;
		G = a[s&1] + 2 - lo;
		if (nc) {
			uint8_t *x = x0 - lo;
			uint64_t *a;
			for (d = lo; d <= hi; ++d) {
				int32_t k = H[d];
				uint8_t z = 1;
				z = k >= H[d+1]? z : 2;
				k = k >= H[d+1]? k : H[d+1];
				++k;
				z = k >= H[d-1]? z : 0;
				k = k >= H[d-1]? k : H[d-1];
				G[d] = k, x[d] = z;
			}
			a = wf_tb_add(&tb, lo, hi);
			for (d = 0; d <= hi - lo; ++d)
				a[d>>5] |= (uint64_t)x0[d] << (d&0x1f)*2;
		} else {
			for (d = lo; d <= hi; ++d) {
				int32_t k = (H[d] >= H[d+1]? H[d] : H[d+1]) + 1;
				G[d] = k >= H[d-1]? k : H[d-1];
			}
		}
		G[lo-2] = G[lo-1] = G[hi+1] = G[hi+2] = WF_NEG_INF;
		H = G;
		while (H[lo] >= tl || lo + H[lo] >= ql) ++lo;
		while (H[hi] >= tl || hi + H[hi] >= ql) --hi;
		++s;
	}
	*ed = s;
	free(a[0]);
	free(pts);
	if (nc) {
		cigar = wf_traceback(tl-1, ts, ql-1, qs, &tb, nc);
		for (i = 0; i < tb.n; ++i) free(tb.a[i].a);
	}
	return cigar;
}
