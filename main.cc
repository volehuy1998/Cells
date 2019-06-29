#include <cmath>
#include <stdio.h>
#include <random> 
#include <vector>
#include <SDL2/SDL.h>
#define DEBUG
//#define LEVEL 3 
constexpr int SCREEN_WIDTH  = 550;
constexpr int SCREEN_HEIGHT = 400;

SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
SDL_bool quit = SDL_FALSE;
SDL_Texture* screen_texture = nullptr;
SDL_Rect screen_rect;

struct Random_machine
{
private:
	std::random_device rd;  
	std::mt19937 gen;
public:
	Random_machine() : gen(rd()) {}
	int get(int a, int b)
	{
		std::uniform_int_distribution<> dis(a, b);
		return dis(gen);
	}
} random_machine;

struct Blob
{
	SDL_Point pos;
	SDL_Point vel;
	float r;
	Blob()
	{
		pos.x = random_machine.get(50, SCREEN_WIDTH - 50);
		pos.y = random_machine.get(50, SCREEN_HEIGHT - 50);
		vel.x = random_machine.get( 3,  7);
		vel.y = random_machine.get(-7, -3);
		r = 50;
	}
	void update()
	{
		pos.x += vel.x;
		pos.y += vel.y;
		if (pos.x + r - 5 > SCREEN_WIDTH  || pos.x - r + 5 < 0) vel.x *= -1;
		if (pos.y + r - 5 > SCREEN_HEIGHT || pos.y - r + 5 < 0) vel.y *= -1;
	}
};

std::vector<Blob*> blobs(7);

int dist(int x1, int y1, int x2, int y2)
{
	int d = sqrt(pow(x1-x2, 2) + pow(y1-y2, 2));
	d = d > 0xff ? 0xff : (d < 0x0 ? 0x0 : d);
	return d;
}

typedef struct RgbColor
{
    unsigned char r;
    unsigned char g;
    unsigned char b;
} RgbColor;

typedef struct HsvColor
{
    unsigned char h;
    unsigned char s;
    unsigned char v;
} HsvColor;

RgbColor HSV2RGB(HsvColor hsv)
{
    RgbColor rgb;
    unsigned char region, remainder, p, q, t;

    if (hsv.s == 0)
    {
        rgb.r = hsv.v;
        rgb.g = hsv.v;
        rgb.b = hsv.v;
        return rgb;
    }

    region = hsv.h / 43;
    remainder = (hsv.h - (region * 43)) * 6; 

    p = (hsv.v * (0xff - hsv.s)) >> 8;
    q = (hsv.v * (0xff - ((hsv.s * remainder) >> 8))) >> 8;
    t = (hsv.v * (0xff - ((hsv.s * (0xff - remainder)) >> 8))) >> 8;

    switch (region)
    {
        case 0:  rgb.r = hsv.v; rgb.g = t;     rgb.b = p;      break;
        case 1:  rgb.r = q; 	rgb.g = hsv.v; rgb.b = p;      break;
        case 2:  rgb.r = p;     rgb.g = hsv.v; rgb.b = t;      break;
        case 3:  rgb.r = p;     rgb.g = q;     rgb.b = hsv.v;  break;
        case 4:  rgb.r = t;     rgb.g = p;     rgb.b = hsv.v;  break;
        default: rgb.r = hsv.v; rgb.g = p;     rgb.b = q;      break;
    }
    return rgb;
}

void setup_success_screen_texture()
{
	void* pixels = nullptr;
	int pitch = 0;
	SDL_LockTexture(screen_texture, nullptr, &pixels, &pitch);
	for (int y = 0; y < SCREEN_HEIGHT; y++)
	{
		for (int x = 0; x < SCREEN_WIDTH; x++)
		{
			int index = y * pitch + x * 4;
			int sum = 0x0;
			for (auto& blob : blobs) {
				int d = dist(x, y, blob->pos.x, blob->pos.y);
				sum += 80 * blobs[0]->r / d;
				if (x == blob->pos.x && y == blob->pos.y && sum < 0xff)
				{
#ifdef DEBUG
					fprintf(stderr, "%d %d %d %d\n", x, y, d, sum);
#endif
					sum = 0xff;
				}

			}
#if LEVEL % 2 == 0
			if (sum > 0xff) sum = 0xff;
#endif
#if LEVEL == 0
			((unsigned char*)pixels)[index + 0] = sum;
			((unsigned char*)pixels)[index + 1] = sum;
			((unsigned char*)pixels)[index + 2] = sum;
#endif
#if LEVEL == 1
			((unsigned char*)pixels)[index + 0] = 0x0;
			((unsigned char*)pixels)[index + 1] = sum;
			((unsigned char*)pixels)[index + 2] = 0x0;
#endif
#if LEVEL == 2 || LEVEL == 3 
			HsvColor hsv;
			hsv.h = sum;
			hsv.s = 0xff;
			hsv.v = 0xff;
			const RgbColor& rgb = HSV2RGB(hsv);
			((unsigned char*)pixels)[index + 0] = rgb.r;
			((unsigned char*)pixels)[index + 1] = rgb.g;
			((unsigned char*)pixels)[index + 2] = rgb.b;
#endif
		}
	}
	SDL_UnlockTexture(screen_texture);
}

void setup_error_screen_texture()
{
	void* pixels = nullptr;
	int pitch = 0;
	int color[] = 
	{ 
		0xff, 0xff, 0xff, // white
		0xff, 0xf0, 0x00, // yellow
		0x00, 0xff, 0xd4, // bad blue
		0x49, 0xff, 0x00, // green
		0xff, 0x00, 0xc9, // purple
		0xff, 0x00, 0x00, // red
		0x00, 0x00, 0xff, // blue
	};
	constexpr int col = 3;
	constexpr int cnt = sizeof color / sizeof(int);
	constexpr int colors = cnt / col;
	constexpr int stride = SCREEN_WIDTH / colors;
	SDL_LockTexture(screen_texture, nullptr, &pixels, &pitch);
	for (int j = 0; j < colors; j++)
	{
		for (int x = j * stride; x < (j + 1) * stride; x++)
		{
			for (int y = 0; y < SCREEN_HEIGHT; y++)
			{
				int index = y * pitch + x * 4;
				((unsigned char*)pixels)[index + 0] = color[(j * 3) % cnt + 0];
				((unsigned char*)pixels)[index + 1] = color[(j * 3) % cnt + 1];
				((unsigned char*)pixels)[index + 2] = color[(j * 3) % cnt + 2];
			}
		}
	}
	SDL_UnlockTexture(screen_texture);

}

int main (int, char**)
{
#ifdef DEBUG
	freopen("out", "w", stderr);
#endif
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		SDL_Log("Error SDL Init: %s", SDL_GetError());
		return 1;
	}

	window = SDL_CreateWindow("Cells", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	screen_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_BGR888, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);
	for (auto& blob : blobs) blob = new Blob();
	while (!quit)
	{
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
				case SDL_QUIT: quit = SDL_TRUE; break;
			}
		}
#ifdef LEVEL
		setup_success_screen_texture();
#else
		setup_error_screen_texture();
#endif
		SDL_RenderCopy(renderer, screen_texture, nullptr, nullptr);
		for (auto& b : blobs) b->update();
		SDL_RenderPresent(renderer);
	}
	
	for (auto& b : blobs) delete b;
	SDL_DestroyTexture(screen_texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}
