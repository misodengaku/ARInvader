

#ifndef __CONTROLLER_H__
#define __CONTROLLER_H__


#define KEY_DOWN	1
#define KEY_UP		0

#define KEY_ESC		27
#define KEY_A		'x'
#define KEY_B		'z'
#define KEY_C		'c'

// コントローラデータ構造体
typedef struct {
	int	up;
	int down;
	int left;
	int right;
	int A;
	int B;
	int C;
} CONTROLLER_DATA;

void GetControllerData( CONTROLLER_DATA *cntl );
void KeyDown( unsigned char key, int x, int y );
void KeyUp( unsigned char key, int x, int y );
void SpecialKeyDown(int key, int x, int y);
void SpecialKeyUp(int key, int x, int y);
void joystick( unsigned int buttonMask, int x, int y, int z );

#endif