extern void print(int);
extern int read();

int func(int n){
	int i;
	i = n;
	while (i > 0){
		print(i);
		i = i - 1;
	}
	return 0;
}
