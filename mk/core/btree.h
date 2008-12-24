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
		BTREE_INSERT_SUB((var), (iter), (iter),	field, cmp);	\
	} while (0)

#define	BTREE_INSERT_SUB(var, iter, node, field, cmp)			\
	do {								\
		(iter) = (node);					\
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

#define	BTREE_REMOVE(var, iter, tree, field)				\
	do {								\
		if ((tree)->child == (var)) {				\
			BTREE_REMOVE_SUB((var), (iter), (tree)->child,	\
					 field);			\
			break;						\
		}							\
									\
		if ((var)->field.parent->field.left == (var)) {		\
			BTREE_REMOVE_SUB((var), (iter),			\
					 (var)->field.parent->field.left,\
					 field);			\
			break;						\
		}							\
		BTREE_REMOVE_SUB((var), (iter),				\
				 (var)->field.parent->field.right,	\
				 field);				\
	} while (0)

#define	BTREE_REMOVE_SUB(var, iter, dst, field)				\
	do {								\
		if ((var)->field.left == NULL) {			\
			(iter) = (var)->field.right;			\
			(dst) = (iter);					\
			if ((iter) != NULL)				\
				(iter)->field.parent =			\
					(var)->field.parent;		\
		} else if ((var)->field.right == NULL) {		\
			(iter) = (var)->field.left;			\
			(dst) = (iter);					\
			(iter)->field.parent = (var)->field.parent;	\
		} else {						\
			(iter) = (var)->field.right;			\
			(dst) = (iter);					\
			(iter)->field.parent = (var)->field.parent;	\
			BTREE_MIN_SUB((iter), (var)->field.right,	\
				      field);				\
			(iter)->field.left = (var)->field.left;		\
			(var)->field.left->field.parent = (iter);	\
		}							\
									\
		(var)->field.parent = NULL;				\
	} while (0)

#endif /* !_CORE_BTREE_H_ */
