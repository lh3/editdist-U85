#include <assert.h>
#include <zlib.h>
#include <stdio.h>
#include <stdlib.h>
#include "ketopt.h"
#include "kalloc.h"
#include "kseq.h"
KSEQ_INIT(gzFile, gzread)

int32_t u85_1(int32_t tl, const char *ts, int32_t ql, const char *qs);
int32_t u85_2(int32_t tl, const char *ts, int32_t ql, const char *qs);
int32_t u85_3(int32_t tl, const char *ts, int32_t ql, const char *qs);
int32_t u85_4(int32_t tl, const char *ts, int32_t ql, const char *qs);

int main(int argc, char *argv[])
{
	gzFile fp1, fp2;
	kseq_t *ks1, *ks2;
	ketopt_t o = KETOPT_INIT;
	int c, algo = 1;

	while ((c = ketopt(&o, argc, argv, 1, "a:", 0)) >= 0) {
		if (o.opt == 'a') algo = atoi(o.arg);
	}
	if (argc - o.ind < 2) {
		fprintf(stderr, "Usage: test-u85 [options] <in1.fa> <in2.fa>\n");
		fprintf(stderr, "Options:\n");
		fprintf(stderr, "  -a INT   algorithm [%d]\n", algo);
		return 1;
	}

	fp1 = gzopen(argv[o.ind+0], "r");
	fp2 = gzopen(argv[o.ind+1], "r");
	assert(fp1 && fp2);
	ks1 = kseq_init(fp1);
	ks2 = kseq_init(fp2);

	while (kseq_read(ks1) >= 0 && kseq_read(ks2) >= 0) {
		int s = -1;
		if (algo == 1) s = u85_1(ks1->seq.l, ks1->seq.s, ks2->seq.l, ks2->seq.s);
		else if (algo == 2) s = u85_2(ks1->seq.l, ks1->seq.s, ks2->seq.l, ks2->seq.s);
		else if (algo == 3) s = u85_3(ks1->seq.l, ks1->seq.s, ks2->seq.l, ks2->seq.s);
		else if (algo == 4) s = u85_4(ks1->seq.l, ks1->seq.s, ks2->seq.l, ks2->seq.s);
		printf("%s\t%s\t%d\n", ks1->name.s, ks2->name.s, s);
	}

	kseq_destroy(ks1);
	kseq_destroy(ks2);
	gzclose(fp1);
	gzclose(fp2);
	return 0;
}
