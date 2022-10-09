/*
 * sccs.h
 *
 * jw 9.2.92
 **************************************************
 * SCCS_ID("@(#)sccs.h	1.2 92/02/21 17:47:52 FAU");
 */

#ifndef __SCCS_H__
# define __SCCS_H__

# if !defined(lint)
#  ifdef __GNUC__
#   define SCCS_ID(id) static char *sccs_id() { return sccs_id(id); }
#  else
#   define SCCS_ID(id) static char *sccs_id = id;
#  endif /* !__GNUC__ */
# else
#  define SCCS_ID(id)      /* Nothing */
# endif /* !lint */

#endif /* __SCCS_H__ */
