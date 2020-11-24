CC      =       nvcc
CFLAGS	=	-g  -lm 
RM 		= rm -f
all: CodeCuda 



CodeCuda:	CodeCuda.cu
	$(CC) -o CodeCuda CodeCuda.cu $(CFLAGS)   

clean:
	$(RM) CodeCuda 
