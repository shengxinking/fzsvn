/*
 *
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "list.h"

typedef struct date {
	int	year;
	int	month;
	int	day;
} date_t;

static int date_cmp(const void *data1, const void *data2)
{
	date_t *p1 = NULL;
	date_t *p2 = NULL;

	if (!data1 && !data2)
		return 0;	

	if (!data1 && data2)
		return -1;

	if (data1 && !data2)
		return 0;

	p1 = (date_t*)data1;
	p2 = (date_t*)data2;

	if (p1->year > p2->year)
		return 1;
	else if (p1->year == p2->year) {
		if (p1->month > p2->month)
			return 1;
		else if (p1->month == p2->month)
			return p1->day - p2->day;
		else
			return -1;
	}
	else
		return -1;
		
	return 0;
}

static int date_find(const void *data1, const void *data2)
{
	date_t *p1 = NULL;
	date_t *p2 = NULL;

	if (!data1 || !data2)
		return -1;

	p1 = (date_t*)data1;
	p2 = (date_t*)data2;

	if (p1->year == p2->year && 
		p1->month == p2->month && 
		p1->day == p2->day)
		return 0;
	else
		return -1;
}

static void date_free(void *data)
{
	if (data)
		free(data);
}

static void date_print(const void *data)
{
	date_t *p1 = NULL;

	if (!data)
		return;

	p1 = (date_t*)data;

	printf("year %d, month %d, day %d\n", p1->year, p1->month, p1->day);
}

static date_t *date_alloc(int year, int month, int day)
{
	date_t *date;

	date = malloc(sizeof(date_t));
	if (!date)
		return NULL;
	
	date->year = year;
	date->month = month;
	date->day = day;

	return date;
}

int main(void)
{
	list_t *list;
	date_t *date;
	int i = 0;

	list = list_alloc(date_cmp, date_find, date_print, date_free);

	for (i = 0; i < 1; i++) {
		date = date_alloc(2007, i, i);
		list_add_tail(list, date);
	}

	list_print(list);

	list_del(list, 0);

	list_print(list);

	list_free(list);

	return 0;
}



