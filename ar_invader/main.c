#include <windows.h>
#include <stdio.h>
#include <string.h>

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>

#include <AR/ar.h>
#include <AR/param.h>
#include <AR/video.h>
#include <AR/gsub.h>

#include "controller.h"	// �L�[�{�[�h�ɂ�鐧��
#include "GLMetaseq.h"	// ���f�����[�_


// ���C�u�����̐ݒ�
#ifdef _DEBUG
	#pragma comment(lib,"libARd.lib")
	#pragma comment(lib,"libARgsubd.lib")
	#pragma comment(lib,"libARvideod.lib")
	#pragma comment(linker,"/NODEFAULTLIB:libcmtd.lib")
#else
	#pragma comment(lib,"libAR.lib")
	#pragma comment(lib,"libARgsub.lib")
	#pragma comment(lib,"libARvideo.lib")
	#pragma comment(linker,"/NODEFAULTLIB:libcmt.lib")
#endif


// �L�����N�^�f�[�^�\����
typedef struct {
	MQO_MODEL	model;
	GLfloat		rot;
	GLfloat		x;
	GLfloat		y;
	GLfloat		z;
} CHARACTER_DATA;


// �C���x�[�_�[�̐�
#define INVADER_COUNT 20

// �X�R�A�\���Ɏg���t�H���g���ƃT�C�Y
// ��肢�������킩���
#define FONT_NAME GLUT_BITMAP_HELVETICA_18
#define FONT_SIZE 18
#define FONT_R 1
#define FONT_G 0
#define FONT_B 0

// �O���[�o���ϐ�
static char	*g_vconf_name	= "Data/WDM_camera_flipV.xml";	// �r�f�I�f�o�C�X�̐ݒ�t�@�C��
static char	*g_cparam_name  = "Data/camera_para.dat";		// �J�����p�����[�^�t�@�C��
static char	*g_patt_name    = "Data/patt.sample1";		// �p�^�[���t�@�C��

static int		g_patt_id;							// �p�^�[����ID
static double	g_patt_trans[3][4];					// ���W�ϊ��s��
static double	g_patt_center[2]	= { 0.0, 0.0 };	// �p�^�[���̒��S���W
static double	g_patt_width		= 80.0;			// �p�^�[���̃T�C�Y�i�P�ʁFmm�j
static int		g_thresh			= 100;			// 2�l����臒l




static CHARACTER_DATA g_airplane = { NULL, 0.0, 0.0, 0.0, 0.0 };	// �L�����N�^�̃f�[�^
static char	*g_mqo_airplane_name	= "Data/mqo/airplane.mqo";			// MQO�t�@�C��

static CHARACTER_DATA g_invader[INVADER_COUNT] = { NULL, 0.0, 0.0, 0.0, 0.0 };		// �C���x�[�_�[
static char	*g_mqo_invader_name = "Data/mqo/invader_small.mqo";
static int invader_show[INVADER_COUNT];
static int invader_x[INVADER_COUNT];
static int invader_y[INVADER_COUNT];
static int invader_z[INVADER_COUNT];

static CHARACTER_DATA g_bullet = { NULL, 0.0, 0.0, 0.0, 0.0 };	// �e��
static char	*g_mqo_bullet_name = "Data/mqo/bullet.mqo";


static int				g_light_on  = TRUE;							// �V�F�[�f�B���O��On/Off
static int				g_optcmf_on = FALSE;						// ���w���ʂ�On/Off
static int				score = 0;									// �X�R�A


// �v���g�^�C�v�錾
static void MainLoop(void);
static void Cleanup(void);
static void DrawObject( ARUint8 *image );
static void CalcState(void);
static void SetLight( GLenum light );
static void OpticalCamouflage( unsigned char *image );
static void GetObjectDepth( GLfloat *depth, int width, int height, GLfloat *depth_min, GLfloat *depth_max );
static void DrawImage( unsigned char *color_buf, int width, int height );
static void DrawString(float x, float y, char* _string);


// ===========================================================================
// main�֐�
// ===========================================================================
int main( int argc, char **argv )
{
	ARParam	cparam;			// �J�����p�����[�^
	ARParam	wparam;			// �J�����p�����[�^�i��Ɨp�ϐ��j
	int		xsize, ysize;	// �摜�T�C�Y
	int i;

	// GLUT�̏�����
	glutInit( &argc, argv );

	// �r�f�I�f�o�C�X�̐ݒ�
	if ( arVideoOpen( g_vconf_name ) < 0 ) {
		printf("�r�f�I�f�o�C�X�̃G���[");
		return -1;
	}

	// �J�����p�����[�^�̐ݒ�
	if ( arVideoInqSize( &xsize, &ysize ) < 0 ) {
		printf("�摜�T�C�Y���擾�ł��܂���ł���\n");
		return -1;
	}
	if ( arParamLoad( g_cparam_name, 1, &wparam ) < 0 ) {
		printf("�J�����p�����[�^�̓ǂݍ��݂Ɏ��s���܂���\n");
		return -1;
	}
	arParamChangeSize( &wparam, xsize, ysize, &cparam );
	arInitCparam( &cparam );

	// �p�^�[���t�@�C���̃��[�h
	if ( (g_patt_id = arLoadPatt(g_patt_name)) < 0 ) {
		printf("�p�^�[���t�@�C���̓ǂݍ��݂Ɏ��s���܂���\n");
		return -1;
	}

	// �E�B���h�E�̐ݒ�
	argInit( &cparam, 1.0, 0, 0, 0, 0 );
	glutSetWindowTitle("AR�C���x�[�_�[");

	// GLMetaseq�̏�����
	mqoInit();

	// ���f���̓ǂݍ���
	if ( ( g_airplane.model = mqoCreateModel( g_mqo_airplane_name, 1.0 )) == NULL ) {
		printf("���f���̓ǂݍ��݂Ɏ��s���܂���\n");
		return -1;
	}

	for (i = 0; i < INVADER_COUNT; i++){
		if ( ( g_invader[i].model = mqoCreateModel( g_mqo_invader_name, 1.0 )) == NULL ) {
			printf("���f���̓ǂݍ��݂Ɏ��s���܂���\n");
			return -1;
		}
		invader_show[i] = 1;
	}

	if ( ( g_bullet.model = mqoCreateModel( g_mqo_bullet_name, 1.0 )) == NULL ) {
		printf("���f���̓ǂݍ��݂Ɏ��s���܂���\n");
		return -1;
	}

	// �r�f�I�L���v�`���̊J�n
	arVideoCapStart();

	// �I�����ɌĂԊ֐����w��
	atexit( Cleanup );

	// �L�[�{�[�h�֐��̓o�^
	glutKeyboardUpFunc( KeyUp );
	glutSpecialFunc( SpecialKeyDown );
	glutSpecialUpFunc( SpecialKeyUp );
	glutJoystickFunc( joystick, 100 ); // �W���C�X�e�B�b�N


	printf("<<������@>>\n");
	printf("�����L�[ : �ړ�\n");
	printf("   %c     : �����ʒu�Ƀ��Z�b�g\n", KEY_B);
	printf("   %c     : �V�F�[�f�B���OOn/Off\n", KEY_A);
	printf("   %c     : ���w����On/Off \n", KEY_C);
	printf("  ESC    : �I��\n");

	// ���C�����[�v�̊J�n
	argMainLoop( NULL, KeyDown, MainLoop );

	return 0;
}


// ===========================================================================
// ���C�����[�v�֐�
// ===========================================================================
static void MainLoop(void)
{
	ARUint8			*image;
	ARMarkerInfo	*marker_info;
	int				marker_num;
	int				j, k;
	char			score_str[64];

	// �J�����摜�̎擾
	if ( (image = arVideoGetImage()) == NULL ) {
		arUtilSleep( 2 );
		return;
	}

	// �J�����摜�̕`��
	argDrawMode2D();
	argDispImage( image, 0, 0 );

	// �}�[�J�̌��o�ƔF��
	if ( arDetectMarker( image, g_thresh, &marker_info, &marker_num ) < 0 ) {
		exit(0);
	}

	// ���̉摜�̃L���v�`���w��
	arVideoCapNext();

	// �}�[�J�̐M���x�̔�r
	k = -1;
	for( j = 0; j < marker_num; j++ ) {
		if ( g_patt_id == marker_info[j].id ) {
			if ( k == -1 ) k = j;
			else if ( marker_info[k].cf < marker_info[j].cf ) k = j;
		}
	}

	if ( k != -1 ) {
		// �}�[�J�̈ʒu�E�p���i���W�ϊ��s��j�̌v�Z
		arGetTransMat( &marker_info[k], g_patt_center, g_patt_width, g_patt_trans );

		// 3D�I�u�W�F�N�g�̕`��
		DrawObject( image );
	}

	// �X�R�A�\���Ɏg�������񏈗�
	sprintf_s(score_str, 64, "score: %d", score);

	// �X�R�A�\��
	DrawString(0.0f, 0.0f, score_str);

	// �o�b�t�@�̓��e����ʂɕ\��
	argSwapBuffers();

}


// ===========================================================================
// �I�������֐�
// ===========================================================================
static void Cleanup(void)
{
    int i;

	arVideoCapStop();	// �r�f�I�L���v�`���̒�~
    arVideoClose();		// �r�f�I�f�o�C�X�̏I��
    argCleanup();		// �O���t�B�b�N�����̏I��

	// ���f���̍폜
	mqoDeleteModel( g_airplane.model );
	mqoDeleteModel( g_bullet.model );
	
	for (i = 0; i < INVADER_COUNT; i++){
		mqoDeleteModel( g_invader[i].model );
	}
	
	mqoCleanup();							// GLMetaseq�̏I������
}


// ===========================================================================
// 3D�I�u�W�F�N�g�̕`����s���֐�
// ===========================================================================
static void DrawObject( ARUint8 *image )
{
	double	gl_para[16];
	int i;
	int invader_row;
	int invader_col;

	// �I�u�W�F�N�g�̏�Ԃ��v�Z
	CalcState();

	// 3D�I�u�W�F�N�g��`�悷�邽�߂̏���
	argDrawMode3D();
	argDraw3dCamera( 0, 0 );

	// �����̐ݒ�
	glPushMatrix();
		SetLight(GL_LIGHT0);
	glPopMatrix();

	// ���W�ϊ��s��̓K�p
	argConvGlpara( g_patt_trans, gl_para );
	glMatrixMode( GL_MODELVIEW );
	glLoadMatrixd( gl_para );

	// 3D�I�u�W�F�N�g�̕`��
	glClear( GL_DEPTH_BUFFER_BIT );		// Z�o�b�t�@�̏�����
	glEnable( GL_DEPTH_TEST );			// �B�ʏ����̓K�p

	// �V�F�[�f�B���O
	if ( g_light_on ) {
		glEnable( GL_LIGHTING );	// ���C�gOn
	} else {
		glDisable( GL_LIGHTING );	// ���C�gOff
	}

	// ���f���̕`��
	glPushMatrix();
		glTranslatef( g_airplane.x, g_airplane.y, g_airplane.z ) ;	// ���f���̈ʒu
		glRotatef( g_airplane.rot, 0.0, 0.0, 1.0);						// ���f���̌���
		glRotatef( 90.0, 1.0, 0.0, 0.0 );								// ���f���𗧂�����
		mqoCallModel( g_airplane.model );								// ���f���̕`��
	glPopMatrix();

	for (i = 0; i < INVADER_COUNT; i++){
		if (invader_show[i] == 1){
			// �C���x�[�_�[�̍s�Ɨ���v�Z
			invader_col = i % 5;
			invader_row = i / 5;

			// �C���x�[�_�[�̍��W���v�Z
			invader_x[i] = invader_col * 80 - 100;
			invader_y[i] = invader_col * 50 - 100;
			invader_z[i] = 300 - invader_row * 60;

			// �`��
			glPushMatrix();			
				glTranslatef( invader_x[i], invader_y[i], invader_z[i] );
				glRotatef( invader_col * 20, 0.0, 0.0, 1.0);
				glRotatef( 90.0, 1.0, 0.0, 0.0 );
				mqoCallModel( g_invader[i].model );
			glPopMatrix();
		}
	}

	glPushMatrix();
		glTranslatef( g_bullet.x, g_bullet.y, g_bullet.z );
		glRotatef( 0.0, 0.0, 0.0, 1.0);
		glRotatef( 90.0, 1.0, 0.0, 0.0 );
		mqoCallModel( g_bullet.model );
	glPopMatrix();


	g_bullet.z += 20;
	for (i = 0; i < INVADER_COUNT; i++){

		// �C���x�[�_�[�Ƃ̏Փ˔���
		if (invader_show[i] == 1
			&& abs(g_bullet.x - invader_x[i]) < 40
			&& abs(g_bullet.y - invader_y[i]) < 20
			&& abs(g_bullet.z - invader_z[i]) < 20){
				// �Փ˂�����
				invader_show[i] = 0;
				g_bullet.x = g_airplane.x;
				g_bullet.y = g_airplane.y;
				g_bullet.z = 40;
				score += 10;
		}
	}

	if (g_bullet.z > 400){
		g_bullet.x = g_airplane.x;
		g_bullet.y = g_airplane.y;
		g_bullet.z = 40;
	}


	glDisable( GL_LIGHTING );
	glDisable( GL_DEPTH_TEST );

	// ���w����
	if ( g_optcmf_on ) {
		OpticalCamouflage( image );
	}
}


// ===========================================================================
// �I�u�W�F�N�g�̏�Ԃ��v�Z����֐�
// ===========================================================================
static void CalcState(void)
{
	CONTROLLER_DATA	control;	// �R���g���[���i�L�[�{�[�h�j
	static GLfloat v = 0;		// �i�s�����̑��x

	// �����I�ɃW���C�X�e�B�b�N��ǂ݂ɍs��
	glutForceJoystickFunc();

	// �R���g���[���̃f�[�^���擾
	GetControllerData( &control );

	if (control.up == KEY_DOWN) {
		g_airplane.y += 10;
	}
	if (control.down == KEY_DOWN) {
		g_airplane.y -= 10;
	}
	if (control.right == KEY_DOWN) {
		g_airplane.x += 10;
	}
	if (control.left == KEY_DOWN) {
		g_airplane.x -= 10;
	}

	/*
	// �����ƌ���
	if ( control.up == KEY_DOWN ) {
		// �������ɉ���
		v = 20;
	} else if ( control.down  == KEY_DOWN ) { 
		// �t�����ɉ���
		v = -20;
	} else {
		// ����
		if (v>0) { v--; }
		else if (v<0) { v++; }
	}

	// ��]
	if ( control.right == KEY_DOWN ) { g_airplane.rot -= 20.0; }
	if ( control.left  == KEY_DOWN ) { g_airplane.rot += 20.0; }

	// �ړ�
	g_airplane.x +=  v * sin(g_airplane.rot*3.14/180);
	g_airplane.y += -v * cos(g_airplane.rot*3.14/180);
	*/

	// �V�F�[�f�B���O��On/Off
	if ( control.A == KEY_DOWN ) g_light_on = !g_light_on;

	// �����ʒu�Ƀ��Z�b�g
	if ( control.B == KEY_DOWN ) {
		g_airplane.rot = 0;
		g_airplane.x = 0;
		g_airplane.y = 0;
		g_airplane.z = 0;
	}

	// ���w���ʂ�On/Off
	if ( control.C == KEY_DOWN ) g_optcmf_on = !g_optcmf_on;

}


// ===========================================================================
// �����̐ݒ���s���֐�
// ===========================================================================
static void SetLight( GLenum light )
{
	GLfloat light_diffuse[]  = { 0.9, 0.9, 0.9, 1.0 };	// �g�U���ˌ�
	GLfloat light_specular[] = { 1.0, 1.0, 1.0, 1.0 };	// ���ʔ��ˌ�
	GLfloat light_ambient[]  = { 0.3, 0.3, 0.3, 0.1 };	// ����
	GLfloat light_position[] = { 0.0, 0.0, 0.0, 1.0 };	// �ʒu�Ǝ��

	// �����̐ݒ�
	glLightfv( light, GL_DIFFUSE,  light_diffuse );	 // �g�U���ˌ��̐ݒ�
	glLightfv( light, GL_SPECULAR, light_specular ); // ���ʔ��ˌ��̐ݒ�
	glLightfv( light, GL_AMBIENT,  light_ambient );	 // �����̐ݒ�
	glLightfv( light, GL_POSITION, light_position ); // �ʒu�Ǝ�ނ̐ݒ�

	glEnable( light );	// �����̗L����
}


// ===========================================================================
// ���w���ʏ������s���֐�
// ===========================================================================
static void OpticalCamouflage( unsigned char *image )
{
	GLfloat			*depth_buf, *pDepth;			// �f�v�X�o�b�t�@
	unsigned char	*color_buf, *pColor;			// �J���[�o�b�t�@
	GLfloat			depth_obj_min, depth_obj_max;	// ���̂̃f�v�X�l�̍ŏ��l�E�ő�l
	GLfloat			depth, depth_obj_n;				// �f�v�X�l�A���̂̐��K���f�v�X�l
	unsigned char	cmfR, cmfG, cmfB;				// ���w���ʂ̐F
	int				width, height;					// �摜�T�C�Y
	int				i, x, y, dx, dy;

	// ���ʂ̐F�i���ʂ̗̈�ɔ�������F�����邽�߂̐ݒ�B���ꂼ��0�`255�j
	cmfR = 255;
	cmfG = 255;
	cmfB = 255;

	// �摜�T�C�Y�̎擾�iARToolKit�̃O���[�o���ϐ�����擾�j
	width = arImXsize;
	height = arImYsize;

	// �f�v�X�o�b�t�@�̎擾
	depth_buf = (GLfloat*)malloc( width * height * sizeof(GLfloat) );
	glReadPixels( 0, 0, width, height, GL_DEPTH_COMPONENT, GL_FLOAT, depth_buf);

	// �`��p�̃o�b�t�@���쐬
	color_buf = (unsigned char*)malloc( width * height * 4 );

	if ( color_buf == NULL || depth_buf == NULL ) return;

	// �I�u�W�F�N�g�̃f�v�X�l�̍ő�ŏ������߂�
	GetObjectDepth( depth_buf, width, height, &depth_obj_min, &depth_obj_max);

	// ���X�^�X�L�����p�̃|�C���^�̏���
	pDepth = depth_buf;
	pColor = color_buf;

	for (y=0; y<height; y++) {
		for (x=0; x<width; x++) {

			// �f�v�X�l
			depth = *pDepth++;

			if ( depth < 1.0 ) {
				// �I�u�W�F�N�g�̈�̏ꍇ

				// �f�v�X�l�𐳋K��
				depth_obj_n = ( depth - depth_obj_min )/( depth_obj_max - depth_obj_min );

				// �f�v�X�l�����ɓǂݎ����W�����炷�i���c�܂���j
				dx = (int)( x -0.05 * width * depth_obj_n * ((x-width/2.0)*0.01) );
				dy = (int)( y -0.05 * height * depth_obj_n * ((y-height/2.0)*0.01) );
				if ( dx < 0 ) dx = x;
				if ( dy < 0 ) dy = y;

				// �C���f�b�N�X�����߂�
				i = 4 * ( (height-1-dy) * width + dx );

				// ��f�l�̏�������
				*pColor++ = (unsigned char)( image[i+2] * 0.9 + cmfR * 0.1 );	// R
				*pColor++ = (unsigned char)( image[i+1] * 0.9 + cmfG * 0.1 );	// G
				*pColor++ = (unsigned char)( image[i]   * 0.9 + cmfB * 0.1 );	// B
				*pColor++ = 255;												// A

			} else {
				// �w�i�̏ꍇ
				pColor += 3;
				*pColor++ = 0;
			}
		}
	}

	// �摜�̕`��
	DrawImage( color_buf, width ,height );

	// �o�b�t�@�̊J��
	free( depth_buf );
	free( color_buf );
}


// ===========================================================================
// �I�u�W�F�N�g�̃f�v�X�l�̍ŏ��l�A�ő�l�����߂�֐�
// depth         : �f�v�X�o�b�t�@
// width, height : �摜�T�C�Y
// depth_min     : �f�v�X�l�̍ŏ��l
// depth_max     : �f�v�X�l�̍ő�l�i�w�i=1.0�������j
// ===========================================================================
static void GetObjectDepth( GLfloat *depth, int width, int height,
							GLfloat *depth_min, GLfloat *depth_max )
{
	GLfloat min = 0;
	GLfloat max = 0;
	GLfloat d;
	int i;

	for ( i=0; i<width*height; i++ ) {

		d = *depth++;

		if ( i==0 ) {
			min = d;
			max = 0;
		} else {
			if ( d > max && d <1.0 ) max = d;
			if ( d < min ) min = d;
		}
	}

	*depth_min = min;
	*depth_max = max;
}


// ===========================================================================
// �摜�̕`����s���֐�
// color_buf     : �J���[�o�b�t�@
// width, height : �摜�T�C�Y
// ===========================================================================
void DrawImage( unsigned char *color_buf, int width, int height )
{
	// �ˉe�s���ۑ�
	glMatrixMode( GL_PROJECTION );
	glPushMatrix();

		// �ˉe�s���ύX
		glLoadIdentity();
		gluOrtho2D( 0, width, 0, height );

		// ���f���r���[�s���ۑ�
		glMatrixMode( GL_MODELVIEW );
		glPushMatrix();

			// �摜�̕`��
			glLoadIdentity();
			glRasterPos2i( 0, 0 );
			glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
			glEnable( GL_BLEND);
			glDrawPixels( width, height, GL_RGBA, GL_UNSIGNED_BYTE, color_buf );
			glDisable( GL_BLEND );

		// ���f���r���[�s��𕜌�
		glPopMatrix();

	// �ˉe�s��𕜌�
	glMatrixMode( GL_PROJECTION );
	glPopMatrix();
}

/*
	�����`��֐�
	http://wiki.livedoor.jp/eruvasu/d/glutSting
*/
static void DrawString(float x, float y, char* _string)
{
	glDisable(GL_LIGHTING);
    // ���s���e�ɂ���
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT), 0);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

	glColor3f(FONT_R, FONT_G, FONT_B);
	glRasterPos2f(x, y + FONT_SIZE);
	//char* p = (char*)_string;
	while (*_string != '\0') glutBitmapCharacter(FONT_NAME, *_string++);
	
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}