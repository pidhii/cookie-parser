$(COOKIE): $(COOKIESRC:%.c=%.o)
	@ar cr $@ $^

