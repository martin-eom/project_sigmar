#define MAP

#ifndef BASE
#include "base.h"
#endif
#ifndef SOLDIERS
#include "soldiers.h"
#endif

#include <vector>

class gridpiece{
	public:
		LinkedList<Soldier*> soldiers;
		LinkedList<gridpiece*> neighbours;
};

class Map {
	public:
		int tilesize;
		int width;
		int heigth;
		int nrows;
		int ncols;
		std::vector<std::vector<gridpiece*>> tiles;
		
		Map(int width, int heigth, int tilesize) {
			this->width = width;
			this->heigth = heigth;
			this->tilesize = tilesize;
			nrows = heigth / tilesize + bool(heigth % tilesize);
			ncols = width / tilesize + bool(width % tilesize);
			tiles = std::vector<std::vector<gridpiece*>>(nrows, std::vector<gridpiece*>(ncols, NULL));
			for(int i = 0; i < nrows; i++) {
				for(int j = 0; j < ncols; j++) {
					tiles.at(i).at(j) = new gridpiece();				
				}
			}
			for(int i = 0; i < nrows - 1; i++) {
				for(int j = 0; j < ncols - 1; j++) {
					tiles.at(i).at(j)->neighbours.Append(new Node<gridpiece*>(tiles.at(i+1).at(j)));
					tiles.at(i).at(j)->neighbours.Append(new Node<gridpiece*>(tiles.at(i).at(j+1)));
					tiles.at(i).at(j)->neighbours.Append(new Node<gridpiece*>(tiles.at(i+1).at(j+1)));
					if(i == 0) {
						tiles.at(nrows-1).at(j)->neighbours.Append(new Node<gridpiece*>(tiles.at(nrows-1).at(j+1)));
					}
				}
				tiles.at(i).at(ncols-1)->neighbours.Append(new Node<gridpiece*>(tiles.at(i+1).at(ncols-1)));
			}
		}
		
		void Cleangrid() {
			for(int i = 0; i < nrows; i++) {
				for(int j = 0; j < ncols; j++) {
					tiles.at(i).at(j)->soldiers.Empty();
				}
			}
		}
		
		void Assign(Soldier* soldier, int i, int j) {
			tiles.at(i).at(j)->soldiers.Append(new Node<Soldier*>(soldier));
		}
};
