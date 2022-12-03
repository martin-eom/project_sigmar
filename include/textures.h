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