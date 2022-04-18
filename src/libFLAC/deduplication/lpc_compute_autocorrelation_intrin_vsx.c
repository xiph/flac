/* This code is imported several times in lpc_intrin_vsx.c with different
 * values for MAX_LAG. Comments are for MAX_LAG == 14 */

long i;
long limit = (long)data_len - MAX_LAG;
const FLAC__real *base;
vector double d0, d1, d2, d3;
vector double sum0 = { 0.0f, 0.0f};
vector double sum10 = { 0.0f, 0.0f};
vector double sum1 = { 0.0f, 0.0f};
vector double sum11 = { 0.0f, 0.0f};
vector double sum2 = { 0.0f, 0.0f};
vector double sum12 = { 0.0f, 0.0f};
vector double sum3 = { 0.0f, 0.0f};
vector double sum13 = { 0.0f, 0.0f};
#if MAX_LAG > 8
vector double d4;
vector double sum4 = { 0.0f, 0.0f};
vector double sum14 = { 0.0f, 0.0f};
#endif
#if MAX_LAG > 10
vector double d5, d6;
vector double sum5 = { 0.0f, 0.0f};
vector double sum15 = { 0.0f, 0.0f};
vector double sum6 = { 0.0f, 0.0f};
vector double sum16 = { 0.0f, 0.0f};
#endif

vector float dtemp;

#if WORDS_BIGENDIAN
vector unsigned long long vperm = { 0x08090A0B0C0D0E0F, 0x1011121314151617 };
vector unsigned long long vsel = { 0x0000000000000000, 0xFFFFFFFFFFFFFFFF };
#else
vector unsigned long long vperm = { 0x0F0E0D0C0B0A0908, 0x1716151413121110 };
vector unsigned long long vsel = { 0xFFFFFFFFFFFFFFFF, 0x0000000000000000 };
#endif

(void) lag;
FLAC__ASSERT(lag <= MAX_LAG);

base = data;

/* First, check whether it is possible to load
 * 16 elements at once */
if(limit > 2){
	/* Convert all floats to doubles */
	dtemp = vec_vsx_ld(0, base);
	d0 = vec_doubleh(dtemp);
	d1 = vec_doublel(dtemp);
	dtemp = vec_vsx_ld(16, base);
	d2 = vec_doubleh(dtemp);
	d3 = vec_doublel(dtemp);
#if MAX_LAG > 8
	dtemp = vec_vsx_ld(32, base);
	d4 = vec_doubleh(dtemp);
#endif
#if MAX_LAG > 10
	d5 = vec_doublel(dtemp);
	dtemp = vec_vsx_ld(48, base);
	d6 = vec_doubleh(dtemp);
#endif

	base += MAX_LAG;

	/* Loop until nearing data_len */
	for (i = 0; i <= (limit-2); i += 2) {
		vector double d, dnext;

		/* Load next 2 datapoints and convert to double
		 * for lag 14 that is data[i+14] and data[i+15] */
		dtemp = vec_vsx_ld(0, base);
		dnext = vec_doubleh(dtemp);
		base += 2;

		/* Create vector d with both elements set to the first
		 * element of d0, so both elements data[i] */
		d = vec_splat(d0, 0);
		sum0 += d0 * d; // Multiply data[i] with data[i] and data[i+1]
		sum1 += d1 * d; // Multiply data[i] with data[i+2] and data[i+3]
		sum2 += d2 * d; // Multiply data[i] with data[i+4] and data[i+5]
		sum3 += d3 * d; // Multiply data[i] with data[i+6] and data[i+7]
#if MAX_LAG > 8
		sum4 += d4 * d; // Multiply data[i] with data[i+8] and data[i+9]
#endif
#if MAX_LAG > 10
		sum5 += d5 * d; // Multiply data[i] with data[i+10] and data[i+11]
		sum6 += d6 * d; // Multiply data[i] with data[i+12] and data[i+13]
#endif

		/* Set both elements of d to data[i+1] */
		d = vec_splat(d0, 1);

		/* Set d0 to data[i+14] and data[i+1] */
		d0 = vec_sel(d0, dnext, vsel);
		sum10 += d0 * d; /* Multiply data[i+1] with data[i+14] and data[i+1] */
		sum11 += d1 * d; /* Multiply data[i+1] with data[i+2] and data[i+3] */
		sum12 += d2 * d;
		sum13 += d3 * d;
#if MAX_LAG > 8
		sum14 += d4 * d;
#endif
#if MAX_LAG > 10
		sum15 += d5 * d;
		sum16 += d6 * d; /* Multiply data[i+1] with data[i+12] and data[i+13] */
#endif

		/* Shift all loaded values one vector (2 elements) so the next
		 * iterations aligns again */
		d0 = d1;
		d1 = d2;
		d2 = d3;
#if MAX_LAG > 8
		d3 = d4;
#endif
#if MAX_LAG > 10
		d4 = d5;
		d5 = d6;
#endif

#if MAX_LAG == 8
		d3 = dnext;
#elif MAX_LAG == 10
		d4 = dnext;
#elif MAX_LAG == 14
		d6 = dnext;
#else
#error "Unsupported lag";
#endif
	}

	/* Because the values in sum10..sum16 do not align with
	 * the values in sum0..sum6, these need to be 'left-rotated'
	 * before adding them to sum0..sum6 */
	sum0 += vec_perm(sum10, sum11, (vector unsigned char)vperm);
	sum1 += vec_perm(sum11, sum12, (vector unsigned char)vperm);
	sum2 += vec_perm(sum12, sum13, (vector unsigned char)vperm);
#if MAX_LAG > 8
	sum3 += vec_perm(sum13, sum14, (vector unsigned char)vperm);
#endif
#if MAX_LAG > 10
	sum4 += vec_perm(sum14, sum15, (vector unsigned char)vperm);
	sum5 += vec_perm(sum15, sum16, (vector unsigned char)vperm);
#endif

#if MAX_LAG == 8
	sum3 += vec_perm(sum13, sum10, (vector unsigned char)vperm);
#elif MAX_LAG == 10
	sum4 += vec_perm(sum14, sum10, (vector unsigned char)vperm);
#elif MAX_LAG == 14
	sum6 += vec_perm(sum16, sum10, (vector unsigned char)vperm);
#else
#error "Unsupported lag";
#endif
}else{
	i = 0;
}

/* Store result */
vec_vsx_st(sum0, 0, autoc);
vec_vsx_st(sum1, 16, autoc);
vec_vsx_st(sum2, 32, autoc);
vec_vsx_st(sum3, 48, autoc);
#if MAX_LAG > 8
vec_vsx_st(sum4, 64, autoc);
#endif
#if MAX_LAG > 10
vec_vsx_st(sum5, 80, autoc);
vec_vsx_st(sum6, 96, autoc);
#endif

/* Process remainder of samples in a non-VSX way */
for (; i < (long)data_len; i++) {
	uint32_t coeff;

	FLAC__real d = data[i];
	for (coeff = 0; coeff < data_len - i; coeff++)
		autoc[coeff] += d * data[i+coeff];
}
