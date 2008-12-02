#ifndef	_CORE_BTREE_H_
#define	_CORE_BTREE_H_

#define	BTREE_NODE(type)						\
	struct {							\
		type *left;						\
		type *right;						\
		type *parent;						\
	}

#define	BTREE_ROOT(type)						\
	struct {							\
		type *child;						\
	}

#define	BTREE_INIT(node)						\
	do {								\
		(node)->left = NULL;					\
		(node)->right = NULL;					\
		(node)->parent = NULL;					\
	} while (0)

#define	BTREE_INIT_ROOT(root)						\
	do {								\
		(root)->child = NULL;					\
	} while (0)

#define	BTREE_INITIALIZER()						\
	{								\
		.left = NULL,						\
		.right = NULL,						\
		.parent = NULL,						\
	}

#define	BTREE_ROOT_INITIALIZER()					\
	{								\
		.child = NULL,						\
	}

#define	BTREE_FIND(hitp, iter, tree, field, cmp, match)			\
	do {								\
		if (((iter) = (tree)->child) == NULL) {			\
			*(hitp) = NULL;					\
			break;						\
		}							\
									\
		for (;;) {						\
			if ((cmp)) {					\
				if ((iter)->field.left == NULL) {	\
					*(hitp) = NULL;			\
					break;				\
				}					\
				(iter) = (iter)->field.left;		\
				continue;				\
			}						\
			if ((match)) {					\
				*(hitp) = (iter);			\
				break;					\
			}						\
			if ((iter)->field.right == NULL) {		\
				*(hitp) = NULL;				\
				break;					\
			}						\
			(iter) = (iter)->field.right;			\
		}							\
	} while (0)

#define	BTREE_FOREACH(var, tree, field, method)				\
	do {								\
		BTREE_MIN((var), (tree), field);			\
									\
		while ((var) != NULL) {					\
			method;						\
			BTREE_NEXT((var), field);			\
		}							\
	} while (0)

#define	BTREE_INSERT(var, iter, tree, field, cmp)			\
	do {								\
		if (((iter) = (tree)->child) == NULL) {			\
			(tree)->child = (var);				\
			(var)->field.parent = (iter);			\
			break;						\
		}							\
									\
		for (;;) {						\
			if ((cmp)) {					\
				if ((iter)->field.left == NULL) {	\
					(iter)->field.left = (var);	\
					(var)->field.parent = (iter);	\
					break;				\
				}					\
				(iter) = (iter)->field.left;		\
				continue;				\
			}						\
			if ((iter)->field.right == NULL) {		\
				(iter)->field.right = (var);		\
				(var)->field.parent = (iter);		\
				break;					\
			}						\
			(iter) = (iter)->field.right;			\
		}							\
	} while (0)

#define	BTREE_MIN_SUB(var, node, field)					\
	do {								\
		if ((node) == NULL) {					\
			(var) = NULL;					\
			break;						\
		}							\
		for ((var) = (node);					\
		     (var)->field.left != NULL;				\
		     (var) = (var)->field.left)				\
			continue;					\
	} while (0)

#define	BTREE_MIN(var, tree, field)					\
	do {								\
		BTREE_MIN_SUB((var), (tree)->child, field);		\
	} while (0)

#define	BTREE_NEXT(var, field)						\
	do {								\
		if ((var)->field.right != NULL) {			\
			BTREE_MIN_SUB((var), (var)->field.right, field);\
			break;						\
		}							\
		for (;;) {						\
			if ((var)->field.parent == NULL) {		\
				(var) = NULL;				\
				break;					\
			}						\
			if ((var)->field.parent->field.left == (var)) {	\
				(var) = (var)->field.parent;		\
				break;					\
			}						\
			(var) = (var)->field.parent;			\
		}							\
	} while (0)

#endif /* !_CORE_BTREE_H_ */
