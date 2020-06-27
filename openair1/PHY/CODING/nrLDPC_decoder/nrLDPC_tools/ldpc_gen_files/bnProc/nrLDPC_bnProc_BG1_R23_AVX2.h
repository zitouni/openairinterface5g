static inline void nrLDPC_bnProc_BG1_R23_AVX2(int8_t* bnProcBuf,int8_t* bnProcBufRes,  int8_t* llrRes, uint16_t Z ) {
        __m256i* p_bnProcBuf; 
        __m256i* p_bnProcBufRes; 
        __m256i* p_llrRes; 
        __m256i* p_res; 
        uint32_t M, i; 
// Process group with 2 CNs 
 M = (1*Z + 31)>>5;
    p_bnProcBuf     = (__m256i*) &bnProcBuf    [3456];
   p_bnProcBufRes    = (__m256i*) &bnProcBufRes   [3456];
            p_res = &p_bnProcBufRes[0];
            p_llrRes = (__m256i*) &llrRes  [3456];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[0 + i]);
             p_res++;
             p_llrRes++;
}
            p_res = &p_bnProcBufRes[12];
            p_llrRes = (__m256i*) &llrRes  [3456];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[12 + i]);
             p_res++;
             p_llrRes++;
}
// Process group with 3 CNs 
       M = (5*Z + 31)>>5;
    p_bnProcBuf     = (__m256i*) &bnProcBuf    [4224];
   p_bnProcBufRes    = (__m256i*) &bnProcBufRes   [4224];
            p_res = &p_bnProcBufRes[0];
            p_llrRes = (__m256i*) &llrRes  [3840];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[0 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[60];
            p_llrRes = (__m256i*) &llrRes  [3840];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[60 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[120];
            p_llrRes = (__m256i*) &llrRes  [3840];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[120 + i]);
            p_res++;
            p_llrRes++;
}
// Process group with 4 CNs 
       M = (3*Z + 31)>>5;
    p_bnProcBuf     = (__m256i*) &bnProcBuf    [9984];
   p_bnProcBufRes    = (__m256i*) &bnProcBufRes   [9984];
            p_res = &p_bnProcBufRes[0];
            p_llrRes = (__m256i*) &llrRes  [5760];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[0 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[36];
            p_llrRes = (__m256i*) &llrRes  [5760];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[36 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[72];
            p_llrRes = (__m256i*) &llrRes  [5760];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[72 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[108];
            p_llrRes = (__m256i*) &llrRes  [5760];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[108 + i]);
            p_res++;
            p_llrRes++;
}
// Process group with 5 CNs 
       M = (7*Z + 31)>>5;
    p_bnProcBuf     = (__m256i*) &bnProcBuf    [14592];
   p_bnProcBufRes    = (__m256i*) &bnProcBufRes   [14592];
            p_res = &p_bnProcBufRes[0];
            p_llrRes = (__m256i*) &llrRes  [6912];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[0 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[84];
            p_llrRes = (__m256i*) &llrRes  [6912];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[84 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[168];
            p_llrRes = (__m256i*) &llrRes  [6912];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[168 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[252];
            p_llrRes = (__m256i*) &llrRes  [6912];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[252 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[336];
            p_llrRes = (__m256i*) &llrRes  [6912];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[336 + i]);
            p_res++;
            p_llrRes++;
}
// Process group with 6 CNs 
       M = (8*Z + 31)>>5;
    p_bnProcBuf     = (__m256i*) &bnProcBuf    [28032];
   p_bnProcBufRes    = (__m256i*) &bnProcBufRes   [28032];
            p_res = &p_bnProcBufRes[0];
            p_llrRes = (__m256i*) &llrRes  [9600];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[0 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[96];
            p_llrRes = (__m256i*) &llrRes  [9600];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[96 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[192];
            p_llrRes = (__m256i*) &llrRes  [9600];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[192 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[288];
            p_llrRes = (__m256i*) &llrRes  [9600];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[288 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[384];
            p_llrRes = (__m256i*) &llrRes  [9600];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[384 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[480];
            p_llrRes = (__m256i*) &llrRes  [9600];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[480 + i]);
            p_res++;
            p_llrRes++;
}
// Process group with 7 CNs 
// Process group with 8 CNs 
// Process group with 9 CNs 
// Process group with 10 CNs 
// Process group with 11 CNs 
       M = (1*Z + 31)>>5;
    p_bnProcBuf     = (__m256i*) &bnProcBuf    [46464];
   p_bnProcBufRes    = (__m256i*) &bnProcBufRes   [46464];
            p_res = &p_bnProcBufRes[0];
            p_llrRes = (__m256i*) &llrRes  [12672];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[0 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[12];
            p_llrRes = (__m256i*) &llrRes  [12672];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[12 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[24];
            p_llrRes = (__m256i*) &llrRes  [12672];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[24 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[36];
            p_llrRes = (__m256i*) &llrRes  [12672];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[36 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[48];
            p_llrRes = (__m256i*) &llrRes  [12672];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[48 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[60];
            p_llrRes = (__m256i*) &llrRes  [12672];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[60 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[72];
            p_llrRes = (__m256i*) &llrRes  [12672];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[72 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[84];
            p_llrRes = (__m256i*) &llrRes  [12672];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[84 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[96];
            p_llrRes = (__m256i*) &llrRes  [12672];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[96 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[108];
            p_llrRes = (__m256i*) &llrRes  [12672];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[108 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[120];
            p_llrRes = (__m256i*) &llrRes  [12672];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[120 + i]);
            p_res++;
            p_llrRes++;
}
// Process group with 12 CNs 
       M = (1*Z + 31)>>5;
    p_bnProcBuf     = (__m256i*) &bnProcBuf    [50688];
   p_bnProcBufRes    = (__m256i*) &bnProcBufRes   [50688];
            p_res = &p_bnProcBufRes[0];
            p_llrRes = (__m256i*) &llrRes  [13056];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[0 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[12];
            p_llrRes = (__m256i*) &llrRes  [13056];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[12 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[24];
            p_llrRes = (__m256i*) &llrRes  [13056];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[24 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[36];
            p_llrRes = (__m256i*) &llrRes  [13056];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[36 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[48];
            p_llrRes = (__m256i*) &llrRes  [13056];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[48 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[60];
            p_llrRes = (__m256i*) &llrRes  [13056];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[60 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[72];
            p_llrRes = (__m256i*) &llrRes  [13056];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[72 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[84];
            p_llrRes = (__m256i*) &llrRes  [13056];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[84 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[96];
            p_llrRes = (__m256i*) &llrRes  [13056];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[96 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[108];
            p_llrRes = (__m256i*) &llrRes  [13056];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[108 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[120];
            p_llrRes = (__m256i*) &llrRes  [13056];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[120 + i]);
            p_res++;
            p_llrRes++;
}
            p_res = &p_bnProcBufRes[132];
            p_llrRes = (__m256i*) &llrRes  [13056];
            for (i=0;i<M;i++) {
            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[132 + i]);
            p_res++;
            p_llrRes++;
}
// Process group with 13 CNs 
// Process group with 14 CNs 
// Process group with 15 CNs 
// Process group with 16 CNs 
// Process group with 17 CNs 
// Process group with 18 CNs 
// Process group with 19 CNs 
// Process group with 20 CNs 
// Process group with 21 CNs 
// Process group with 22 CNs 
// Process group with <23 CNs 
// Process group with 24 CNs 
// Process group with 25 CNs 
// Process group with 26 CNs 
// Process group with 27 CNs 
// Process group with 28 CNs 
// Process group with 29 CNs 
// Process group with 30 CNs 
}
