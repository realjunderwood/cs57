extern void print(int);
extern int read();

int func(int n){
	int result;
	if (n < 0)
		result = 0 - n;
	else
		result = n;
	return result;
}
