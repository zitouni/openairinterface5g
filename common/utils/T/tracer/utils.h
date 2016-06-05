#ifndef _UTILS_H_
#define _UTILS_H_

void new_thread(void *(*f)(void *), void *data);
void sleepms(int ms);

/****************************************************************************/
/* list                                                                     */
/****************************************************************************/

typedef struct list {
  struct list *last, *next;
  void *data;
} list;

list *list_remove_head(list *l);
list *list_append(list *l, void *data);

/****************************************************************************/
/* socket                                                                   */
/****************************************************************************/

/* socket_send: return 0 if okay, -1 on error */
int socket_send(int socket, void *buffer, int size);
int get_connection(char *addr, int port);
int fullread(int fd, void *_buf, int count);
int connect_to(char *addr, int port);

/****************************************************************************/
/* buffer                                                                   */
/****************************************************************************/

typedef struct {
  int osize;
  int omaxsize;
  char *obuf;
} OBUF;

void PUTC(OBUF *o, char c);
void PUTS(OBUF *o, char *s);
void PUTS_CLEAN(OBUF *o, char *s);
void PUTI(OBUF *o, int i);
void PUTUL(OBUF *o, unsigned long i);

#endif /* _UTILS_H_ */
