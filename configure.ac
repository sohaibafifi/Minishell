
AC_INIT([minishell], [0.1],[mailto:me@sohaibafifi.com],[minishell])

AM_INIT_AUTOMAKE([1.9 foreign dist-bzip2 no-dist-gzip -Wall])

AC_CONFIG_SRCDIR([src])


AC_CHECK_HEADER(readline/readline.h,READLINE_LIBS="-w -s /usr/lib/libreadline.so /usr/lib/libhistory.so",AC_MSG_ERROR("readline/readline.h not found."))
#AC_CONFIG_HEADERS(readline/readline.h,READLINE_LIBS="-w -s /usr/lib/libreadline.so /usr/lib/libhistory.so",AC_MSG_ERROR("readline/readline.h not found."))
#AC_CHECK_LIB(readline,,AC_MSG_ERROR("readline not found."))
AC_SUBST(READLINE_LIBS)


AC_PROG_CC
AC_HEADER_STDC
AC_CHECK_FUNCS([dup2 putenv getcwd strchr strdup strerror strrchr])


AC_CHECK_HEADERS([fcntl.h]) 
AC_FUNC_FORK 
AC_FUNC_MALLOC 
AC_TYPE_PID_T 
AC_TYPE_SIZE_T 

AC_CONFIG_FILES([
	Makefile
	src/Makefile 
])

AC_OUTPUT

