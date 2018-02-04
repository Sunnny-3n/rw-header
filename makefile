cc = cc
prom = proxy
cflag = -Wall -g -Og
ldflags = 
deps = $(shell find ./ -name "*.h")
src = $(shell find ./ -name "*.c")
obj = $(src:%.c=%.o)

$(prom): $(obj)
	$(cc) $(cflag) $(ldflags) -o $(prom) $(obj)

%.o: %.c $(deps)
	$(cc) $(cflag) -c $< -o $@

.PHONY : clean
clean:
	-rm -rf $(obj) $(prom) ./compile_commands.json


.PHONY : analysis
analysis:
	@-rm ./compile_commands.json
	@make clean
	@bear make 
	@python2 ./run-clang-tidy.py

.PHONY : run
run:
	@make -s
	@./$(prom)
