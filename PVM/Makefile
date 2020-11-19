CC      =       cc
CFLAGS	=	-g  -lm -lpvm3
RM 		= rm -f
all: LanceTaches Tache 



LanceTaches:	CodeMaitre.c
	$(CC) -o CodeMaitre CodeMaitre.c $(CFLAGS)   

Tache:	Tache.c 
	$(CC)  -o Tache Tache.c  $(CFLAGS)  
	cp Tache $(HOME)

clean:
	$(RM) LanceTaches 
	$(RM) Tache 
	$(RM) $(HOME)/Tache
