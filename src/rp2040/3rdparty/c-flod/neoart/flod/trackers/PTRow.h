#ifndef PTROW_H
#define PTROW_H

#include "../core/AmigaRow.h"

/*
inheritance
Object
	-> AmigaRow
		-> PTRow
*/
struct __attribute__((packed)) PTRow {
	struct AmigaRow super;
	short step;
};

void PTRow_defaults(struct PTRow* self);
void PTRow_ctor(struct PTRow* self);
struct PTRow* PTRow_new(void);

#endif
