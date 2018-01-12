PROGRAM = ewolutionarisher_taschenmensch

CFLAGS =\
	-Wall\
	-Werror\
	-fopenmp\
	-g
LFLAGS =\
	-lomp

OBJS =\
	main.o

bin/$(PROGRAM) : $(OBJS) | bin
	gcc -o $@ $(CFLAGS) $<

obj/%.o : %.c | obj
	gcc -o $@ -c $(CFLAGs) $< 

bin obj : 
	mkdir -p $@

clean : 
	rm -rf bin obj

run : bin/$(PROGRAM)
	bin/$(PROGRAM)

.PHONY: clean