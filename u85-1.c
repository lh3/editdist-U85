#include <stdlib.h>
#include <stdint.h>

#define WF_NEG_INF (-0x40000000)

int32_t u85_1(int32_t tl, const char *ts, int32_t ql, const char *qs)
{
	int32_t *a[2], *H, lo = 0, hi = 0, s = 0;
	a[0] = (int32_t*)malloc((tl + ql + 5) * 2 * sizeof(int32_t));
	a[1] = a[0] + tl + ql + 5;
	H = a[1] + 2;
	H[0] = -1, H[-2] = H[-1] = H[1] = H[2] = WF_NEG_INF; // pad with WF_NEG_INF
	while (1) {
		int32_t d, *G;
		for (d = lo; d <= hi; ++d) {
			int32_t k = H[d], i = d + k;
			if (k < -1 || i < -1) continue;
			while (k + 1 < tl && i + 1 < ql && ts[k+1] == qs[i+1])
				++k, ++i;
			if (k == tl - 1 && i == ql - 1) break;
			H[d] = k;
		}
		if (d <= hi) break;
		while (H[lo] >= tl || lo + H[lo] >= ql) ++lo;
		while (H[hi] >= tl || hi + H[hi] >= ql) --hi;
		if (lo > -tl) --lo;
		if (hi <  ql) ++hi;
		G = a[s&1] + 2 - lo;
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
	return s;
}
