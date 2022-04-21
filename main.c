#include <assert.h>
#include <zlib.h>
#include <stdio.h>
#include <stdlib.h>
#include "ketopt.h"
#include "kseq.h"
KSEQ_INIT(gzFile, gzread)

int32_t u85_basic(int32_t tl, const char *ts, int32_t ql, const char *qs);
int32_t u85_fast(int32_t tl, const char *ts, int32_t ql, const char *qs);
uint32_t *u85_cigar(int32_t tl, const char *ts, int32_t ql, const char *qs, int32_t *ed, int32_t *nc);

int main(int argc, char *argv[])
{
	gzFile fp1, fp2;
	kseq_t *ks1, *ks2;
	ketopt_t o = KETOPT_INIT;
	int32_t c, i, use_fast = 0, report_cigar = 0, n_cigar = 0;
	uint32_t *cigar = 0;

	while ((c = ketopt(&o, argc, argv, 1, "fc", 0)) >= 0) {
		if (o.opt == 'f') use_fast = 1;
		else if (o.opt == 'c') report_cigar = 1;
	}
	if (argc - o.ind < 2) {
		fprintf(stderr, "Usage: test-u85 [-c] [-f] <in1.fa> <in2.fa>\n");
		return 1;
	}

	fp1 = gzopen(argv[o.ind+0], "r");
	fp2 = gzopen(argv[o.ind+1], "r");
	assert(fp1 && fp2);
	ks1 = kseq_init(fp1);
	ks2 = kseq_init(fp2);

	while (kseq_read(ks1) >= 0 && kseq_read(ks2) >= 0) {
		int s = -1;
		if (report_cigar) cigar = u85_cigar(ks1->seq.l, ks1->seq.s, ks2->seq.l, ks2->seq.s, &s, &n_cigar);
		else if (use_fast) s = u85_fast(ks1->seq.l, ks1->seq.s, ks2->seq.l, ks2->seq.s);
		else s = u85_basic(ks1->seq.l, ks1->seq.s, ks2->seq.l, ks2->seq.s);
		printf("%s\t%s\t%d", ks1->name.s, ks2->name.s, s);
		if (report_cigar) {
			putchar('\t');
			for (i = 0; i < n_cigar; ++i)
				printf("%d%c", cigar[i]>>4, "MIDNSHP=XB"[cigar[i]&0xf]);
		}
		putchar('\n');
	}

	kseq_destroy(ks1);
	kseq_destroy(ks2);
	gzclose(fp1);
	gzclose(fp2);
	return 0;
}
