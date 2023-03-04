#ifndef TEXTURE
#define TEXTURE

#include <SDL.h>
#include <SDL_ttf.h>
#include <string>

class Texture {
public:
	int _width;
	int _length;
	SDL_Surface* surface;
	SDL_Texture* texture;
	SDL_Renderer* renderer;
public:
	void free() {
		if(surface) SDL_FreeSurface(surface);
		if(texture) SDL_DestroyTexture(texture);
		surface = NULL; texture = NULL;
		_width = 0; _length = 0;
	}	
	
	Texture(SDL_Renderer* renderer) {
		surface = NULL; texture = NULL;
		_width = 0; _length = 0;
		this->renderer = renderer;
	};
	~Texture() {free();}

	int width() {return _width;}
	int length() {return _length;}
	void render(int x, int y, double angle = 0.0, SDL_Point* center = NULL, SDL_Rect* clip = NULL, SDL_RendererFlip flip = SDL_FLIP_NONE) {
		SDL_Rect rec = {x, y, _width, _length};
		SDL_RenderCopyEx(renderer, texture, clip, &rec, angle, center, flip);
	};


};

class ImgTexture : public Texture{
public:
	ImgTexture(SDL_Renderer* renderer) : Texture(renderer) {}

	void loadFromImage(const char* filename) {
		surface = SDL_LoadBMP(filename);
		if(surface == NULL) {
			std::printf("Could not load image. SDL Error %s\n", SDL_GetError());
		}
		else {
			texture = SDL_CreateTextureFromSurface(renderer, surface);
			if(texture == NULL) {
				std::printf("Unable to create texture from image surface. SDL Error %s\n", SDL_GetError());
			}
			else {
				_width = surface->w;
				_length = surface->h;
			}
		}
	}

	void renderZoomed(double x, double y, double rad, int SCREEN_WIDTH, int SCREEN_HEIGHT, double zoom, Eigen::Vector2d mapCenter, double angle = 0.0, SDL_Point* center = NULL, SDL_Rect* clip = NULL, SDL_RendererFlip flip = SDL_FLIP_NONE) {
		Eigen::Vector2d diag; diag << SCREEN_WIDTH / 2., SCREEN_HEIGHT / 2.;
		double rrad = zoom * rad;
		double xx = zoom*x - mapCenter.coeff(0) + diag.coeff(0);
		double yy = zoom*y - mapCenter.coeff(1) + diag.coeff(1);
		yy = SCREEN_HEIGHT - yy;
		SDL_Rect rec;
		SDL_Point point;
		SDL_Point* destCenter = &point;
		if(clip) {
			double cx, cy;
			if(center) {
				cx = center->x; cy = center->y;
			}
			else {
				cx = clip->w / 2.; cy = clip->h / 2.;
			}
			rec.x = xx - cx / 32 * 2 * rrad; rec.y = yy - cy / 32 * 2 * rrad; rec.w = clip->w / 32 * 2 * rrad; rec.h = clip->h / 32 * 2 * rrad;
			point.x = cx / 32 * 2 * rrad; point.y = cy / 32 * 2 * rrad;
		}
		else {
			rec.x = xx-rrad; rec.y = yy-rrad; rec.w = 2*rrad; rec.h = 2*rrad;
			destCenter = NULL;
			//rec << {xx - rrad,yy - rrad,2*rrad,2*rrad};
		}
		SDL_RenderCopyEx(renderer, texture, clip, &rec, angle, destCenter, flip);
	};

};

class StringTexture : public Texture{
public:
	StringTexture(SDL_Renderer* renderer) : Texture(renderer) {}

	void loadFromString(std::string s, TTF_Font* font, SDL_Color color, int textwidth) {
		free();
		if(!font) {
			std::cout << "There is no font.\n";
		}
		surface = TTF_RenderText_Solid_Wrapped(font, s.c_str(), color, textwidth);
		if(surface == NULL) {
			std::printf("Could not load string texture. SDL Error: %s\n",TTF_GetError());
		}
		else {
			texture = SDL_CreateTextureFromSurface(renderer, surface);
			if(texture == NULL) {
				std::printf("Unable to create texture from rendered text! SDL Error: %s\n", SDL_GetError());
			}
			else {
				_width = surface->w;
				_length = surface->h;
			}
		}
	}
};

#endif