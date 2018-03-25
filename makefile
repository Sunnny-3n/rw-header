CC = cc
PROM = proxy
CFLAG = -Wall -O3
LDFLAGS = 
DEPS = $(shell find ./ -name "*.h")
SRC = $(shell find ./ -name "*.c")
OBJ = $(SRC:%.c=%.o)

$(PROM): $(OBJ)
	$(CC) $(CFLAG) $(LDFLAGS) -o $(PROM) $(OBJ)

%.o: %.c $(DEPS)
	$(CC) $(CFLAG) -c $< -o $@

.PHONY : clean
clean:
	-rm -rf $(OBJ) $(PROM)


.PHONY : analysis
analysis:
	@-rm ./compile_commands.json
	@make clean
	@bear make 
	@python2 ./tool/run-clang-tidy.py

.PHONY : run
run:
	@make -s
	@./$(PROM)
