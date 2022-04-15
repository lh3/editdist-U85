This repo provides an efficient implementation of the [Ukkonen85
algorithm][U85], which is sometimes also known as [Myers' O(ND)
algorithm][myers86] or the [Landau-Vishkin algorithm][lv89]. Please see the
[notes here][note] for a historical background.

There are two implementations. [u85-basic.c](u85-basic.c) gives a basic
implementation. It is more complex but can be faster than the original
algorithm when input sequences are distinct in length. [u85-fast.c](u85-fast.c)
is an optimized version. It is comparable to [edlib][edlib] in speed on a few
examples.

[U85]: https://www.sciencedirect.com/science/article/pii/S0019995885800462
[myers86]: https://link.springer.com/article/10.1007/BF01840446
[lv89]: https://doi.org/10.1016/0196-6774(89)90010-2
[note]: https://github.com/lh3/miniwfa#historical-notes-on-wfa-and-related-algorithms
[edlib]: https://github.com/Martinsos/edlib
