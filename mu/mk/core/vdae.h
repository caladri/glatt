#ifndef	_CORE_VDAE_H_
#define	_CORE_VDAE_H_

struct vdae;
struct vdae_list;

typedef	void (vdae_callback_t)(void *);

struct vdae *vdae_create(struct vdae_list *, vdae_callback_t *, void *);
struct vdae_list *vdae_list_create(const char *name);
void vdae_list_wakeup(struct vdae_list *);

#endif /* !_CORE_VDAE_H_ */
