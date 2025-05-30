/*
 * p11test_case_ec_sign.c: Test different data lengths for EC signatures
 *
 * Copyright (C) 2016, 2017 Red Hat, Inc.
 *
 * Author: Jakub Jelen <jjelen@redhat.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "p11test_case_ec_sign.h"
#include "libopensc/internal.h"

void ec_sign_size_test(void **state) {
	unsigned int i;
	unsigned long min, max, l;
	int inc, errors = 0, rv;
	size_t j;
	token_info_t *info = (token_info_t *) *state;
	test_certs_t objects;

	test_certs_init(&objects);

	P11TEST_START(info);
	if (token.num_ec_mechs == 0 && token.num_ed_mechs == 0) {
		fprintf(stderr, "Token does not support any ECC signature mechanisms. Skipping.\n");
		P11TEST_SKIP(info);
	}

	search_for_all_objects(&objects, info);

	debug_print("\nCheck functionality of Sign&Verify on different data lengths");
	for (i = 0; i < objects.count; i++) {
		unsigned long curve_len = 0;
		switch (objects.data[i].key_type) {
		case CKK_EC:
			/* This tests just couple of sizes around the curve length
			 * to verify they are properly truncated on input */
			curve_len = BYTES4BITS(objects.data[i].bits);
			min = curve_len - 2;
			max = curve_len + 2;
			inc = 1;
			break;
		case CKK_EC_EDWARDS:
			/* Tests larger inputs for EdDSA. Previously, we had hardcoded limit of 512
			 * https://github.com/OpenSC/OpenSC/issues/2300 */
			min = 128;
			max = 1024;
			inc = 128;
			break;
		default:
			continue;
		}

		// sanity: Test all mechanisms
		if (objects.data[i].sign && objects.data[i].verify) {
			for (j = 0; j < objects.data[i].num_mechs; j++) {
				test_mech_t *m = &(objects.data[i].mechs[j]);
				if ((m->usage_flags & CKF_SIGN) == 0) {
					/* Skip non-signature mechanisms (for example derive ones) */
					continue;
				}
				for (l = min; l < max; l += inc) {
					/* Skip inputs not matching digest sizes for raw ECDSA as the card
					 * will likely reject them as not valid hash outputs */
					if (m->mech == CKM_ECDSA && (l != 20 && l != 28 && l != 32 && l != 48 && l != 64))
						continue;
					rv = sign_verify_test(&(objects.data[i]), info, m, l, 0);
					if (rv == -1)
						errors++;
				}
			}
		}
	}
	clean_all_objects(&objects);

	if (errors > 0)
		P11TEST_FAIL(info, "Some signatures were not verified successfully. Please review the log");
	P11TEST_PASS(info);
}
