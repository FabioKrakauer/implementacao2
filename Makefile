vm: frk.c
	gcc $< -o $@ -pthread

.PHONY: run clean
run:
	./vm addresses.txt fifo fifo
clean:
	rm vm