// berry-dm
// Copyright © 2022 Yuichiro Nakada

#include <termios.h>

struct termios orig_termios;

void reset_terminal_mode()
{
	tcsetattr(0, TCSANOW, &orig_termios);
}

void set_conio_terminal_mode()
{
	struct termios new_termios;

	/* take two copies - one for now, one for later */
	tcgetattr(0, &orig_termios);
	memcpy(&new_termios, &orig_termios, sizeof(new_termios));

	/* register cleanup handler, and set the new terminal mode */
	atexit(reset_terminal_mode);
	cfmakeraw(&new_termios);
	new_termios.c_lflag &= ~ICANON;
	new_termios.c_lflag &= ~ECHO;
	new_termios.c_cc[VMIN] = 0;	// 0文字入力された時点で入力を受け取る
	new_termios.c_cc[VTIME] = 0;	// 何も入力がない場合、1/10秒待つ
	tcsetattr(0, TCSANOW, &new_termios);
}

#include "kms-glsl.h"

static const struct egl *egl;
static const struct gbm *gbm;
static const struct drm *drm;

//void main(){vec4 p;for(int i=0;i<30;++i)p=vec4(gl_FragCoord.xy/r-.5,1,0)*o,p.z-=5.,o+=length(p)-1.;o*=.1;}

uint64_t ui_glsl_peek_event()
{
	uint64_t c = 0;
	read(0, &c, 8);
#ifdef DEBUG
	printf("%lx\n", c);
#endif
	return c;
}

void ui_glsl_draw(uint8_t *data)
{
//	glClearColor(0.5, 0.5, 0.5, 1.0);
//	glClear(GL_COLOR_BUFFER_BIT);

	if (!sampler_channel_ID[3]) glGenTextures(1, &sampler_channel_ID[3]);
	update_texture(3, data, 256, 3);

	drm->draw(gbm, egl);
}

int ui_glsl_init()
{
	const char *device = NULL;
	char mode_str[DRM_DISPLAY_MODE_LEN] = "";
	unsigned int vrefresh = 0;
	unsigned int count = ~0;
	int atomic = 0;
	uint32_t format = DRM_FORMAT_XRGB8888;
	uint64_t modifier = DRM_FORMAT_MOD_LINEAR;
	bool surfaceless = false;

	if (atomic) {
		drm = init_drm_atomic(device, mode_str, vrefresh, count);
	} else {
		drm = init_drm_legacy(device, mode_str, vrefresh, count);
	}
	if (!drm) {
		printf("failed to initialize %s DRM\n", atomic ? "atomic" : "legacy");
		return -1;
	}

	gbm = init_gbm(drm->fd, drm->mode->hdisplay, drm->mode->vdisplay, format, modifier, surfaceless);
	if (!gbm) {
		printf("failed to initialize GBM\n");
		return -1;
	}

	egl = init_egl(gbm);
	if (!egl) {
		printf("failed to initialize EGL\n");
		return -1;
	}

	int ret = init_shadertoy(gbm, (struct egl *)egl, GLSL);
	if (ret < 0) {
		return -1;
	}

//	texture_bind(data, 256, 3);

	set_conio_terminal_mode();
	return 0;
}

void ui_glsl_shutdown()
{
}
