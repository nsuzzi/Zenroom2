/* This file is part of Zenroom (https://zenroom.dyne.org)
 *
 * Copyright (C) 2023 Dyne.org foundation
 * designed, written and maintained by Denis Roio <jaromil@dyne.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include <p256-m.h>

#include <zen_error.h>
#include <zen_octet.h>
#include <zenroom.h>
#include <lua_functions.h>

#define PK_SIZE 64
#define UNCOMPRESSED_PK_SIZE 65
#define COMPRESSED_PK_SIZE 33
#define PK_COORD_SIZE 32
#define SK_SIZE 32
#define HASH_SIZE 32
#define SIG_SIZE 64

#define ASSERT_OCT_LEN(OCT, SIZE, MSG) \
	if ((OCT)->len != SIZE)        \
	{                              \
		failed_msg = (MSG);    \
		lua_pushnil(L);        \
		goto end;              \
	}


static int allocate_raw_public_key(lua_State *L, int pk_pos, octet **res_pk, char **failed_msg) {
	const octet* pk = o_arg(L, pk_pos);
	if (!pk) {
		*failed_msg = "Could not allocate public key";
		return 1;
	}
	*res_pk = o_alloc(L, PK_SIZE);
	if(*res_pk == NULL) {
		o_free(L, pk);
		*failed_msg = "Could not allocate raw public key";
		return 1;
	}
	(*res_pk)->len = PK_SIZE;
	if (pk->len == PK_SIZE) {
		for(uint8_t i=0; i<PK_SIZE; i++) (*res_pk)->val[i] = pk->val[i];
		o_free(L, pk);
		return 0;
	}
	if (pk->len == UNCOMPRESSED_PK_SIZE) {
		// Check for correct prefix in long public key
		if (pk->val[0] != 0x04) {
			*failed_msg = "Invalid long public key prefix: 0x04 expected";
			o_free(L, pk);
			return 1;
		}
		for(uint8_t i=0; i<PK_SIZE; i++) (*res_pk)->val[i] = pk->val[i+1];
		o_free(L, pk);
		return 0;
	}
	if (pk->len == COMPRESSED_PK_SIZE) {
		// Handle compressed public key
		if (pk->val[0] != 0x02 && pk->val[0] != 0x03) {
			*failed_msg = "Invalid compressed public key prefix: 0x02 or 0x03 expected";
			o_free(L, pk);
			return 1;
		}
		int res = p256_uncompress_publickey((uint8_t*)(*res_pk)->val, (uint8_t*)pk->val);
		o_free(L, pk);
		return res;
	}
	o_free(L, pk);
	*failed_msg = "Invalid public key length";
	return 1;
}

static int p256_keygen(lua_State *L)
{
	BEGIN();
	Z(L);
	uint8_t pub[PK_SIZE];
	octet *sk = o_new(L, SK_SIZE);
	sk->len = SK_SIZE;
	if(!sk) {
		THROW("Could not allocate secret key");
	} else {
		p256_gen_keypair(Z, NULL, (uint8_t*)sk->val, pub);
	}
	END(1);
}

static int p256_pubgen(lua_State *L)
{
	BEGIN();
	char *failed_msg = NULL;
	const octet *sk = NULL;
	octet *pk = NULL;
	sk = o_arg(L, 1);
	if (!sk)
	{
		failed_msg = "Could not allocate secret key";
		goto end;
	}

	ASSERT_OCT_LEN(sk, SK_SIZE, "Invalid size for P256 secret key")

	pk = o_new(L, PK_SIZE);
	if (!pk)
	{
		failed_msg = "Could not allocate public key";
		goto end;
	}
	pk->len = PK_SIZE;

	int ret = p256_publickey((uint8_t *)sk->val, (uint8_t *)pk->val);
	if (ret != 0)
	{
		failed_msg = "Could not generate public key";
		goto end;
	}
end:
	o_free(L, sk);
	if (failed_msg != NULL)
	{
		THROW(failed_msg);
	}
	END(1);
}

static int p256_session(lua_State *L)
{
	BEGIN();
	// TODO: is there a KEM session function in our p256 lib?
	END(1);
}

static int p256_pubcheck(lua_State *L) {
	BEGIN();
	octet *raw_pk = NULL;
	char *failed_msg = NULL;
	int ret = allocate_raw_public_key(L, 1, &raw_pk, &failed_msg);
	if (ret != 0) goto end;
	lua_pushboolean(L, p256_validate_pubkey((uint8_t*)raw_pk->val)==0);
end:
	o_free(L, raw_pk);
	if (failed_msg != NULL) {
		THROW(failed_msg);
	}
	END(1);
}

static int p256_sign(lua_State *L)
{
	BEGIN();
	Z(L);

	int n_args = lua_gettop(L);
	hash256 sha256;
	char hash[HASH_SIZE];
	char *failed_msg = NULL;
	const octet *sk = NULL, *m = NULL, *k = NULL;
	octet *sig = NULL;
	sk = o_arg(L, 1);
	if (!sk)
	{
		failed_msg = "Could not allocate secret key";
		goto end;
	}
	m = o_arg(L, 2);
	if (!m)
	{
		failed_msg = "Could not allocate message";
		goto end;
	}

	ASSERT_OCT_LEN(sk, SK_SIZE, "Invalid size for ECDSA secret key")
	HASH256_init(&sha256);
	for (int i = 0; i < m->len; i++)
	{
		HASH256_process(&sha256, m->val[i]);
	}
	HASH256_hash(&sha256, hash);

	sig = o_new(L, SIG_SIZE);
	if (!sig)
	{
		failed_msg = "Could not allocate signature";
		goto end;
	}
	sig->len = SIG_SIZE;

	if(n_args > 2) {
		k = o_arg(L, 3);
		if(k == NULL) {
			failed_msg = "Could not allocate ephemeral key";
			goto end;
		}
	}

	int ret = p256_ecdsa_sign(Z, k, (uint8_t *)sig->val, (uint8_t *)sk->val,
				  (uint8_t *)hash, HASH_SIZE);
	if (ret != 0)
	{
		failed_msg = "Could not sign message";
		goto end;
	}

end:
	o_free(L, m);
	o_free(L, sk);
	o_free(L, k);
	if (failed_msg != NULL)
	{
		THROW(failed_msg);
	}
	END(1);
}

static int p256_verify(lua_State *L)
{
	BEGIN();
	hash256 sha256;
	char hash[HASH_SIZE];
	char *failed_msg = NULL;
	octet *raw_pk = NULL;
	const octet *sig = NULL, *m = NULL;
	int ret = allocate_raw_public_key(L, 1, &raw_pk, &failed_msg);
	if (ret != 0) goto end;
	m = o_arg(L, 2);
	if (!m)
	{
		failed_msg = "Could not allocate message";
		goto end;
	}
	sig = o_arg(L, 3);
	if (!sig)
	{
		failed_msg = "Could not allocate signature";
		goto end;
	}

	ASSERT_OCT_LEN(sig, SIG_SIZE, "Invalid size for P256 signature")

	HASH256_init(&sha256);
	for (int i = 0; i < m->len; i++)
	{
		HASH256_process(&sha256, m->val[i]);
	}
	HASH256_hash(&sha256, hash);

	lua_pushboolean(L, p256_ecdsa_verify((uint8_t *)sig->val,
					     (uint8_t *)raw_pk->val,
					     (uint8_t *)hash, HASH_SIZE) == 0);
end:
	o_free(L, m);
	o_free(L, sig);
	o_free(L, raw_pk);
	if (failed_msg != NULL)
	{
		THROW(failed_msg);
	}
	END(1);
}

static int p256_pub_xy(lua_State *L) {
	BEGIN();
	char *failed_msg = NULL;
	octet *raw_pk = NULL;
	int i;
	int ret = allocate_raw_public_key(L, 1, &raw_pk, &failed_msg);
	if (ret != 0) goto end;
	octet *x = o_new(L, PK_COORD_SIZE+1);
	if(x == NULL) {
		failed_msg = "Could not create x coordinate";
		goto end;
	}
	for(i=0; i < PK_COORD_SIZE; i++)
		x->val[i] = raw_pk->val[i];
	x->val[PK_COORD_SIZE+1] = 0x0;
	x->len = PK_COORD_SIZE;
	octet *y = o_new(L, PK_COORD_SIZE+1);
	if(y == NULL) {
		failed_msg = "Could not create y coordinate";
		goto end;
	}
	for(i=0; i < PK_COORD_SIZE; i++)
		y->val[i] = raw_pk->val[PK_COORD_SIZE+i];
	y->val[PK_COORD_SIZE+1] = 0x0;
	y->len = PK_COORD_SIZE;
end:
	o_free(L, raw_pk);
	if(failed_msg) {
		THROW(failed_msg);
		lua_pushnil(L);
	}
	END(2);
}

static int p256_destroy(lua_State *L)
{
	BEGIN();
	END(0);
}

static int p256_compress_pub(lua_State *L) {
	BEGIN();
	char *failed_msg = NULL;
	octet *raw_pk = NULL;
	int ret = allocate_raw_public_key(L, 1, &raw_pk, &failed_msg);
	if (ret != 0) goto end;
	octet *compressed_pk = o_new(L, COMPRESSED_PK_SIZE);
	if(compressed_pk == NULL) {
		failed_msg = "Could not create compressed public key";
		goto end;
	}
	compressed_pk->len = COMPRESSED_PK_SIZE;
	ret = p256_compress_publickey((uint8_t*)compressed_pk->val, (uint8_t*)raw_pk->val);
	if (ret != 0) {
		failed_msg = "Could not compress public key";
		goto end;
	}
end:
	o_free(L, raw_pk);
	if(failed_msg) {
		THROW(failed_msg);
	}
	END(1);
}

int luaopen_p256(lua_State *L)
{
	(void)L;
	const struct luaL_Reg p256_class[] = {
	    {"keygen", p256_keygen},
	    {"pubgen", p256_pubgen},
	    {"session", p256_session},
	    {"checkpub", p256_pubcheck},
	    {"pubcheck", p256_pubcheck},
	    {"validate", p256_pubcheck},
	    {"sign", p256_sign},
	    {"verify", p256_verify},
	    {"public_xy", p256_pub_xy},
	    {"pubxy", p256_pub_xy},
		{"compress_public_key", p256_compress_pub},
	    {NULL, NULL}};
	const struct luaL_Reg p256_methods[] = {
	    {"__gc", p256_destroy},
	    {NULL, NULL}};

	zen_add_class(L, "p256", p256_class, p256_methods);
	return 1;
}
