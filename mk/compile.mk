%.o: %.c $(CINCLUDEFILES)
	gcc -c $(CFLAGS) -I $(CINCLUDE) -o $@ $<

