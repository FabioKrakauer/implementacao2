vm: frk.c
	gcc $< -o $@ -pthread

.PHONY: run clean
run:
	./vm addresses.txt lru fifo
clean:
	rm vm