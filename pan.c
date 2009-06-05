#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <time.h>
#include <linux/fb.h>
#include <linux/omapfb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#define FBCTL0(ctl) if (ioctl(fd, ctl))\
	{ perror("fbctl0(" __FILE__ ":" TOSTRING(__LINE__) "): "); exit(1); }
#define FBCTL1(ctl, arg1) if (ioctl(fd, ctl, arg1))\
	{ perror("fbctl1(" __FILE__ ":" TOSTRING(__LINE__) "): "); exit(1); }

static int screen_w;
static int screen_h;
static int bytespp;

int main(void)
{
	int fd;
	int frame;
	struct timeval tv1, tv2, tv;
	struct timeval ftv1, ftv2;
	struct fb_var_screeninfo var;
	struct omapfb_mem_info mi;
	struct omapfb_plane_info pi;
	void *fb_base;
	unsigned long min_pan_us, max_pan_us, sum_pan_us;

	fd = open("/dev/fb0", O_RDWR);

	FBCTL1(FBIOGET_VSCREENINFO, &var);
	screen_w = var.xres;
	screen_h = var.yres;
	bytespp = var.bits_per_pixel / 8;

	FBCTL1(OMAPFB_QUERY_PLANE, &pi);
	pi.enabled = 0;
	FBCTL1(OMAPFB_SETUP_PLANE, &pi);

	FBCTL1(OMAPFB_QUERY_MEM, &mi);
	mi.size = screen_w * screen_h * bytespp * 2;
	FBCTL1(OMAPFB_SETUP_MEM, &mi);

	FBCTL1(FBIOGET_VSCREENINFO, &var);
	var.yres_virtual = var.yres * 2;
	FBCTL1(FBIOPUT_VSCREENINFO, &var);

	pi.enabled = 1;
	FBCTL1(OMAPFB_SETUP_PLANE, &pi);

	fb_base = mmap(NULL, mi.size, PROT_READ | PROT_WRITE, MAP_SHARED,
			fd, 0);
	if (fb_base == MAP_FAILED) {
		perror("mmap: ");
		exit(1);
	}

	frame = 0;
	gettimeofday(&ftv1, NULL);
	min_pan_us = 0xffffffff;
	max_pan_us = 0;
	sum_pan_us = 0;
	while (1) {
		const int num_frames = 100;
		unsigned long us;
		void *fb;

		if (frame > 0 && frame % num_frames == 0) {
			unsigned long ms;
			gettimeofday(&ftv2, NULL);
			timersub(&ftv2, &ftv1, &tv);

			ms = tv.tv_sec * 1000 + tv.tv_usec / 1000;

			printf("%d frames in %lu ms, %lu fps, "
					"pan min %lu, max %lu, avg %lu\n",
					num_frames,
					ms, num_frames * 1000 / ms,
					min_pan_us, max_pan_us,
					sum_pan_us / num_frames);

			gettimeofday(&ftv1, NULL);
			min_pan_us = 0xffffffff;
			max_pan_us = 0;
			sum_pan_us = 0;
		}

		if (frame % 2 == 0)
			var.yoffset = 0;
		else
			var.yoffset = var.yres;
		frame++;

		gettimeofday(&tv1, NULL);

		FBCTL1(FBIOPAN_DISPLAY, &var);

		gettimeofday(&tv2, NULL);
		timersub(&tv2, &tv1, &tv);

		us = tv.tv_sec * 1000000 + tv.tv_usec;

		if (us > max_pan_us)
			max_pan_us = us;
		if (us < min_pan_us)
			min_pan_us = us;
		sum_pan_us += us;

		FBCTL0(OMAPFB_WAITFORVSYNC);


		if (var.yoffset != 0)
			fb = fb_base;
		else
			fb = fb_base + screen_w * screen_h * bytespp;

		if (1)
		{
			int x, y, i;
			unsigned int *p32 = fb;
			unsigned short *p16 = fb;
			x = frame % (screen_w - 20);

			memset(fb, 0xff, screen_w*screen_h*bytespp);

			for (i = 0; i < 20; ++i) {
				for (y = 0; y < screen_h; ++y) {
					if (bytespp == 2)
						p16[y * screen_w + x] =
							random();
					else
						p32[y * screen_w + x] =
							random();
				}
				msync(fb, screen_w*screen_h*bytespp, MS_SYNC);
			}
		}

		if (1) {
			int x, y, i;
			unsigned int *p32 = fb;
			unsigned short *p16 = fb;
			memset(fb, 0, screen_w*screen_h*bytespp);
			x = frame % (screen_w - 20);
			for (y = 0; y < screen_h; ++y) {
				for (i = x; i < x+20; ++i) {
					if (bytespp == 2)
						p16[y * screen_w + i] =
							0xffff;
					else
						p32[y * screen_w + i] =
							0xffffffff;
				}
			}
			msync(fb, screen_w*screen_h*bytespp, MS_SYNC);
		}
	}

	close(fd);

	munmap(fb_base, mi.size);

	return 0;
}

