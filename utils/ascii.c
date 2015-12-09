// clang ascii.c -o ascii
#include <stdio.h>
#include <termios.h>

void main() {
	struct termios term;
	struct termios save;
	tcgetattr(0, &term);
	//ioctl(0, TCGETA, &term);
	save = term;
	term.c_lflag &= ~ICANON;
	term.c_lflag &= ~ECHO;
	term.c_cc[VMIN] = 0;	// 0文字入力された時点で入力を受け取る
	term.c_cc[VTIME] = 5;	// 何も入力がない場合、5/10秒待つ
	tcsetattr(0, TCSANOW, &term);

	long long c;
	do {
		c = 0;
		read(0, &c, 8);
		printf("0x%016llX\n", c);
	} while (1);

	tcsetattr(0, TCSANOW, &save);
}
