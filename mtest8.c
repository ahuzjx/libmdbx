/* mtest8.c - memory-mapped database tester/toy */
/*
 * Copyright 2015 Ilya Usvyatsky, Nexenta Corp.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in the file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */

/* Tests for DB attributes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "mdbx.h"

#define E(expr) CHECK((rc = (expr)) == MDB_SUCCESS, #expr)
#define RES(err, expr) ((rc = expr) == (err) || (CHECK(!rc, #expr), 0))
#define CHECK(test, msg) ((test) ? (void)0 : ((void)fprintf(stderr, \
	"%s:%d: %s: %s\n", __FILE__, __LINE__, msg, mdb_strerror(rc)), abort()))

char dkbuf[1024];

int main(int argc,char * argv[])
{
	int i = 0, rc;
	MDB_env *env;
	MDB_dbi dbi;
	MDB_val key, data, data1;
	MDB_txn *txn;
	MDB_stat mst;
	int count;
	int *values;
	char sval[8000];
	uint64_t *timestamps, timestamp;
	struct timeval tv;
	int env_opt = MDB_NOMEMINIT | MDB_NOSYNC | MDB_NOSUBDIR | MDB_NORDAHEAD;

	srand(time(NULL));

	memset(sval, 0, sizeof(sval));
	count = 2000; //(rand()%384) + 64;
	if (argc > 1)
		count = atoi(argv[1]);
	values = (int *)malloc(count*sizeof(int));
	timestamps = (uint64_t *)calloc(count,sizeof(uint64_t));

	key.mv_size = sizeof(int);
	data.mv_size = sizeof(sval);
	data.mv_data = sval;

	values[0] = 42;
	values[1] = 17;

	for (i = 2; i < count; ++i)
		values[i] = values[i - 1] + values[i - 2];

	E(mdb_env_create(&env));
	E(mdb_env_set_mapsize(env, 104857600));
	E(mdb_env_set_maxdbs(env, 8));
	E(mdb_env_open(env, "./mtest8.db", env_opt, 0664));

	E(mdb_txn_begin(env, NULL, 0, &txn));
	E(mdb_dbi_open(txn, "id8", MDB_CREATE|MDB_INTEGERKEY, &dbi));

	for (i = 0; i < count; ++i) {
		(void)gettimeofday(&tv, NULL);
		timestamps[i] = tv.tv_usec + 1000000UL * tv.tv_sec;

		snprintf(sval, 4000, "Value %d\n", values[i]);
		snprintf(sval + 4000, 4000, "Value %d\n", values[i]);
		key.mv_data = values + i;
		E(mdb_put_attr(txn, dbi, &key, &data, timestamps[i],
			    MDB_NODUPDATA));
	}

	E(mdb_txn_commit(txn));
	E(mdb_env_stat(env, &mst));

	mdb_dbi_close(env, dbi);
	mdb_env_close(env);

	E(mdb_env_create(&env));
	E(mdb_env_set_mapsize(env, 10485760));
	E(mdb_env_set_maxdbs(env, 8));
	E(mdb_env_open(env, "./mtest8.db", env_opt, 0664));

	E(mdb_txn_begin(env, NULL, 0, &txn));
	E(mdb_dbi_open(txn, "id8", MDB_CREATE|MDB_INTEGERKEY, &dbi));
	for (i = 0; i < count; ++i) {
		key.mv_data = values + i;
		E(mdb_get_attr(txn, dbi, &key, &data, &timestamp));
		E(timestamps[i] != timestamp);

		E(mdb_get(txn, dbi, &key, &data1));
		E(data.mv_size != data1.mv_size);
		E(memcmp(data.mv_data, data1.mv_data, data.mv_size));
	}

	E(mdb_txn_commit(txn));
	E(mdb_env_stat(env, &mst));

	mdb_dbi_close(env, dbi);
	mdb_env_close(env);

	E(mdb_env_create(&env));
	E(mdb_env_set_mapsize(env, 104857600));
	E(mdb_env_set_maxdbs(env, 8));
	E(mdb_env_open(env, "./mtest8.db", env_opt, 0664));

	E(mdb_txn_begin(env, NULL, 0, &txn));
	E(mdb_dbi_open(txn, "id8", MDB_CREATE|MDB_INTEGERKEY, &dbi));

	for (i = 0; i < count; ++i) {
		(void)gettimeofday(&tv, NULL);
		timestamps[i] = tv.tv_usec + 1000000UL * tv.tv_sec;

		key.mv_data = values + i;
		E(mdb_set_attr(txn, dbi, &key, NULL, timestamps[i]));
	}

	E(mdb_txn_commit(txn));
	E(mdb_env_stat(env, &mst));

	mdb_dbi_close(env, dbi);
	mdb_env_close(env);

	E(mdb_env_create(&env));
	E(mdb_env_set_mapsize(env, 10485760));
	E(mdb_env_set_maxdbs(env, 8));
	E(mdb_env_open(env, "./mtest8.db", env_opt, 0664));

	E(mdb_txn_begin(env, NULL, 0, &txn));
	E(mdb_dbi_open(txn, "id8", MDB_CREATE|MDB_INTEGERKEY, &dbi));
	for (i = 0; i < count; ++i) {
		key.mv_data = values + i;
		E(mdb_get_attr(txn, dbi, &key, &data, &timestamp));
		E(timestamps[i] != timestamp);

		E(mdb_get(txn, dbi, &key, &data1));
		E(data.mv_size != data1.mv_size);
		E(memcmp(data.mv_data, data1.mv_data, data.mv_size));
	}

	E(mdb_txn_commit(txn));
	E(mdb_env_stat(env, &mst));

	mdb_dbi_close(env, dbi);
	mdb_env_close(env);

	return 0;
}
