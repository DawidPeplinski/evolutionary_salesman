PROGRAM = ewolutionarisher_taschenmensch

CFLAGS =\
	-g

OBJS =\
	main.o

bin/$(PROGRAM) : $(OBJS) | bin
	gcc -o $@ $<

obj/%.o : %.c | obj
	gcc -o $@ -c $(CFLAGs) $< 

bin obj : 
	mkdir -p $@

clean : 
	rm -rf bin obj

run : bin/$(PROGRAM)
	bin/$(PROGRAM)

.PHONY: clean