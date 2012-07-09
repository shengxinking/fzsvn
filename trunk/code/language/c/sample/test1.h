/*
 *	test1.h:	baidu test header file
 *
 */

#ifndef _TEST1_H
#define _TEST1_H

struct item {
	int id;
	int weight;
};

struct update {
	int id;
	int old_weight;
	int new_weight;
};

extern update(struct item *l1, int n1, const struct update *l2, int n2);

#endif /* end of _TEST1_H */

