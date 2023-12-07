
#include "gtest/gtest.h"
using namespace std;
// hacks private fields
#define private public
#define protected public
#include "util/util.h"
TEST(TestFastaRead, testFastaRead) {
    string s;
    readFastaFile("test_data/test.fna",s);
    ASSERT_EQ(
        s,
        "GCTGCATGATATTGAAAAAATATCACCAAATAAAAAACGCCTTAGTAAGTATTTTTCAGCTTTTCATTCTGACTGCAACGGGCAATATGTCTCTGTGTGGATTAAAAAAGAGTGTCTGATAGCAGCTTCTGAACTGGTTACCTGCCGTGAGTAAATTAAAATTTTATTGACTTAGGTCACTAAATACTTTAACCAATATAGGCATAGCGCACAGACAGATAAAAATTACAGAGTACACAACATCCATGAAACGCATTAGCACCACCATTACCACCACCATCACCATTACCACAGGTAACGGTGCGGGCTGACGACGTACAGGAAACACAGAAAAAAGCCCGCTAC");
}