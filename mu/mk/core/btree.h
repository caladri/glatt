#ifndef	_CORE_BTREE_H_
#define	_CORE_BTREE_H_

#define	BTREE(type)							\
	struct {							\
		type *left;						\
		type *right;						\
	}

#define	BTREE_INIT(tree)						\
	do {								\
		(tree)->left = NULL;					\
		(tree)->right = NULL;					\
	} while (0)

/* XXX These are temporary for helping transition code.  */
#define	BTREE_LEFT(tree)						\
	((tree)->left)
#define	BTREE_RIGHT(tree)						\
	((tree)->right)

#endif /* !_CORE_BTREE_H_ */
