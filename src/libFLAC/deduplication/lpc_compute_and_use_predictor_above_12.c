/* This code is ordered such that each loop is at least 2/3ths unrolled */

if(order <= 22) {
	if(order <= 16) {
		for(i = 0; i < (int)data_len; i++) {
			sum = 0;
			switch(order) {
				case 16: sum += qlp_coeff[15] * (FLAC__LPC_DATA_TYPE)data[i-16]; /* Falls through. */
				case 15: sum += qlp_coeff[14] * (FLAC__LPC_DATA_TYPE)data[i-15]; /* Falls through. */
				case 14: sum += qlp_coeff[13] * (FLAC__LPC_DATA_TYPE)data[i-14]; /* Falls through. */
				case 13: sum += qlp_coeff[12] * (FLAC__LPC_DATA_TYPE)data[i-13];
						 sum += qlp_coeff[11] * (FLAC__LPC_DATA_TYPE)data[i-12];
						 sum += qlp_coeff[10] * (FLAC__LPC_DATA_TYPE)data[i-11];
						 sum += qlp_coeff[ 9] * (FLAC__LPC_DATA_TYPE)data[i-10];
						 sum += qlp_coeff[ 8] * (FLAC__LPC_DATA_TYPE)data[i- 9];
						 sum += qlp_coeff[ 7] * (FLAC__LPC_DATA_TYPE)data[i- 8];
						 sum += qlp_coeff[ 6] * (FLAC__LPC_DATA_TYPE)data[i- 7];
						 sum += qlp_coeff[ 5] * (FLAC__LPC_DATA_TYPE)data[i- 6];
						 sum += qlp_coeff[ 4] * (FLAC__LPC_DATA_TYPE)data[i- 5];
						 sum += qlp_coeff[ 3] * (FLAC__LPC_DATA_TYPE)data[i- 4];
						 sum += qlp_coeff[ 2] * (FLAC__LPC_DATA_TYPE)data[i- 3];
						 sum += qlp_coeff[ 1] * (FLAC__LPC_DATA_TYPE)data[i- 2];
						 sum += qlp_coeff[ 0] * (FLAC__LPC_DATA_TYPE)data[i- 1];
			}
			FLAC__LPC_ACTION;
		}
	}
	else { /* 16 < order <= 20 */
		for(i = 0; i < (int)data_len; i++) {
			sum = 0;
			switch(order) {
				case 22: sum += qlp_coeff[21] * (FLAC__LPC_DATA_TYPE)data[i-22]; /* Falls through. */
				case 21: sum += qlp_coeff[20] * (FLAC__LPC_DATA_TYPE)data[i-21]; /* Falls through. */
				case 20: sum += qlp_coeff[19] * (FLAC__LPC_DATA_TYPE)data[i-20]; /* Falls through. */
				case 19: sum += qlp_coeff[18] * (FLAC__LPC_DATA_TYPE)data[i-19]; /* Falls through. */
				case 18: sum += qlp_coeff[17] * (FLAC__LPC_DATA_TYPE)data[i-18]; /* Falls through. */
				case 17: sum += qlp_coeff[16] * (FLAC__LPC_DATA_TYPE)data[i-17];
				         sum += qlp_coeff[15] * (FLAC__LPC_DATA_TYPE)data[i-16];
						 sum += qlp_coeff[14] * (FLAC__LPC_DATA_TYPE)data[i-15];
						 sum += qlp_coeff[13] * (FLAC__LPC_DATA_TYPE)data[i-14];
						 sum += qlp_coeff[12] * (FLAC__LPC_DATA_TYPE)data[i-13];
						 sum += qlp_coeff[11] * (FLAC__LPC_DATA_TYPE)data[i-12];
						 sum += qlp_coeff[10] * (FLAC__LPC_DATA_TYPE)data[i-11];
						 sum += qlp_coeff[ 9] * (FLAC__LPC_DATA_TYPE)data[i-10];
						 sum += qlp_coeff[ 8] * (FLAC__LPC_DATA_TYPE)data[i- 9];
						 sum += qlp_coeff[ 7] * (FLAC__LPC_DATA_TYPE)data[i- 8];
						 sum += qlp_coeff[ 6] * (FLAC__LPC_DATA_TYPE)data[i- 7];
						 sum += qlp_coeff[ 5] * (FLAC__LPC_DATA_TYPE)data[i- 6];
						 sum += qlp_coeff[ 4] * (FLAC__LPC_DATA_TYPE)data[i- 5];
						 sum += qlp_coeff[ 3] * (FLAC__LPC_DATA_TYPE)data[i- 4];
						 sum += qlp_coeff[ 2] * (FLAC__LPC_DATA_TYPE)data[i- 3];
						 sum += qlp_coeff[ 1] * (FLAC__LPC_DATA_TYPE)data[i- 2];
						 sum += qlp_coeff[ 0] * (FLAC__LPC_DATA_TYPE)data[i- 1];
			}
			FLAC__LPC_ACTION;
		}
	}
}
else { /* order > 22 */
	if(order <= 28) {
		for(i = 0; i < (int)data_len; i++) {
			sum = 0;
			switch(order) {
				case 28: sum += qlp_coeff[27] * (FLAC__LPC_DATA_TYPE)data[i-28]; /* Falls through. */
				case 27: sum += qlp_coeff[26] * (FLAC__LPC_DATA_TYPE)data[i-27]; /* Falls through. */
				case 26: sum += qlp_coeff[25] * (FLAC__LPC_DATA_TYPE)data[i-26]; /* Falls through. */
				case 25: sum += qlp_coeff[24] * (FLAC__LPC_DATA_TYPE)data[i-25]; /* Falls through. */
				case 24: sum += qlp_coeff[23] * (FLAC__LPC_DATA_TYPE)data[i-24]; /* Falls through. */
				case 23: sum += qlp_coeff[22] * (FLAC__LPC_DATA_TYPE)data[i-23];
						 sum += qlp_coeff[21] * (FLAC__LPC_DATA_TYPE)data[i-22];
						 sum += qlp_coeff[20] * (FLAC__LPC_DATA_TYPE)data[i-21];
						 sum += qlp_coeff[19] * (FLAC__LPC_DATA_TYPE)data[i-20];
						 sum += qlp_coeff[18] * (FLAC__LPC_DATA_TYPE)data[i-19];
						 sum += qlp_coeff[17] * (FLAC__LPC_DATA_TYPE)data[i-18];
						 sum += qlp_coeff[16] * (FLAC__LPC_DATA_TYPE)data[i-17];
						 sum += qlp_coeff[15] * (FLAC__LPC_DATA_TYPE)data[i-16];
						 sum += qlp_coeff[14] * (FLAC__LPC_DATA_TYPE)data[i-15];
						 sum += qlp_coeff[13] * (FLAC__LPC_DATA_TYPE)data[i-14];
						 sum += qlp_coeff[12] * (FLAC__LPC_DATA_TYPE)data[i-13];
						 sum += qlp_coeff[11] * (FLAC__LPC_DATA_TYPE)data[i-12];
						 sum += qlp_coeff[10] * (FLAC__LPC_DATA_TYPE)data[i-11];
						 sum += qlp_coeff[ 9] * (FLAC__LPC_DATA_TYPE)data[i-10];
						 sum += qlp_coeff[ 8] * (FLAC__LPC_DATA_TYPE)data[i- 9];
						 sum += qlp_coeff[ 7] * (FLAC__LPC_DATA_TYPE)data[i- 8];
						 sum += qlp_coeff[ 6] * (FLAC__LPC_DATA_TYPE)data[i- 7];
						 sum += qlp_coeff[ 5] * (FLAC__LPC_DATA_TYPE)data[i- 6];
						 sum += qlp_coeff[ 4] * (FLAC__LPC_DATA_TYPE)data[i- 5];
						 sum += qlp_coeff[ 3] * (FLAC__LPC_DATA_TYPE)data[i- 4];
						 sum += qlp_coeff[ 2] * (FLAC__LPC_DATA_TYPE)data[i- 3];
						 sum += qlp_coeff[ 1] * (FLAC__LPC_DATA_TYPE)data[i- 2];
						 sum += qlp_coeff[ 0] * (FLAC__LPC_DATA_TYPE)data[i- 1];
			}
			FLAC__LPC_ACTION;
		}
	}
	else { /* order > 28 */
		for(i = 0; i < (int)data_len; i++) {
			sum = 0;
			switch(order) {
				case 32: sum += qlp_coeff[31] * (FLAC__LPC_DATA_TYPE)data[i-32]; /* Falls through. */
				case 31: sum += qlp_coeff[30] * (FLAC__LPC_DATA_TYPE)data[i-31]; /* Falls through. */
				case 30: sum += qlp_coeff[29] * (FLAC__LPC_DATA_TYPE)data[i-30]; /* Falls through. */
				case 29: sum += qlp_coeff[28] * (FLAC__LPC_DATA_TYPE)data[i-29];
						 sum += qlp_coeff[27] * (FLAC__LPC_DATA_TYPE)data[i-28];
						 sum += qlp_coeff[26] * (FLAC__LPC_DATA_TYPE)data[i-27];
						 sum += qlp_coeff[25] * (FLAC__LPC_DATA_TYPE)data[i-26];
						 sum += qlp_coeff[24] * (FLAC__LPC_DATA_TYPE)data[i-25];
						 sum += qlp_coeff[23] * (FLAC__LPC_DATA_TYPE)data[i-24];
						 sum += qlp_coeff[22] * (FLAC__LPC_DATA_TYPE)data[i-23];
						 sum += qlp_coeff[21] * (FLAC__LPC_DATA_TYPE)data[i-22];
						 sum += qlp_coeff[20] * (FLAC__LPC_DATA_TYPE)data[i-21];
						 sum += qlp_coeff[19] * (FLAC__LPC_DATA_TYPE)data[i-20];
						 sum += qlp_coeff[18] * (FLAC__LPC_DATA_TYPE)data[i-19];
						 sum += qlp_coeff[17] * (FLAC__LPC_DATA_TYPE)data[i-18];
						 sum += qlp_coeff[16] * (FLAC__LPC_DATA_TYPE)data[i-17];
						 sum += qlp_coeff[15] * (FLAC__LPC_DATA_TYPE)data[i-16];
						 sum += qlp_coeff[14] * (FLAC__LPC_DATA_TYPE)data[i-15];
						 sum += qlp_coeff[13] * (FLAC__LPC_DATA_TYPE)data[i-14];
						 sum += qlp_coeff[12] * (FLAC__LPC_DATA_TYPE)data[i-13];
						 sum += qlp_coeff[11] * (FLAC__LPC_DATA_TYPE)data[i-12];
						 sum += qlp_coeff[10] * (FLAC__LPC_DATA_TYPE)data[i-11];
						 sum += qlp_coeff[ 9] * (FLAC__LPC_DATA_TYPE)data[i-10];
						 sum += qlp_coeff[ 8] * (FLAC__LPC_DATA_TYPE)data[i- 9];
						 sum += qlp_coeff[ 7] * (FLAC__LPC_DATA_TYPE)data[i- 8];
						 sum += qlp_coeff[ 6] * (FLAC__LPC_DATA_TYPE)data[i- 7];
						 sum += qlp_coeff[ 5] * (FLAC__LPC_DATA_TYPE)data[i- 6];
						 sum += qlp_coeff[ 4] * (FLAC__LPC_DATA_TYPE)data[i- 5];
						 sum += qlp_coeff[ 3] * (FLAC__LPC_DATA_TYPE)data[i- 4];
						 sum += qlp_coeff[ 2] * (FLAC__LPC_DATA_TYPE)data[i- 3];
						 sum += qlp_coeff[ 1] * (FLAC__LPC_DATA_TYPE)data[i- 2];
						 sum += qlp_coeff[ 0] * (FLAC__LPC_DATA_TYPE)data[i- 1];
			}
			FLAC__LPC_ACTION;
		}
	}
}