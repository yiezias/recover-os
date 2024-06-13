#ifndef __LIB_LIST_H
#define __LIB_LIST_H
#include "types.h"


#define elem2entry(struct_name, member_name, member_ptr) \
	(struct_name *)((uint64_t)member_ptr             \
			- ((uint64_t) & ((struct_name *)0)->member_name))

struct list_elem {
	struct list_elem *prev;
	struct list_elem *next;
};

struct list {
	struct list_elem head;
	struct list_elem tail;
};

/* 初始化链表结构 */
void list_init(struct list *plist);
/* 链表是否为空 */
bool list_empty(struct list *plist);
/* 将inserted插入pelem之前 */
void list_insert_before(struct list_elem *pelem, struct list_elem *inserted);
/* 移除pelem */
void list_remove(struct list_elem *pelem);
/* 表头插入pelem */
void list_push(struct list *plist, struct list_elem *pelem);
/* 表尾连接pelem */
void list_append(struct list *plist, struct list_elem *pelem);
/* 表头弹出一个元素 */
struct list_elem *list_pop(struct list *plist);
/* 判断元素是否在链表内 */
bool elem_find(struct list *plist, struct list_elem *pelem);

#endif
