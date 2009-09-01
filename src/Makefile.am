bin_PROGRAMS = minishell 

minishell_SOURCES = main.c main.h rline.c rline.h

minishell_CPPFLAGS = 				\
	-DMAXELEMS=32
minishell_CFLAGS = \
	   -DMAXELEMS=32   
CFLAGS = \
	    -g -O0	

	 


minishell_LDADD = @READLINE_LIBS@ 	
