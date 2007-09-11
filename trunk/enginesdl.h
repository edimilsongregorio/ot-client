//////////////////////////////////////////////////////////////////////
// Yet Another Tibia Client
//////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//////////////////////////////////////////////////////////////////////

#ifndef __ENGINESDL_H
#define __ENGINESDL_H

#include "engine.h"
#include "spritesdl.h"
class EngineSDL : public Engine
{
public:
    EngineSDL();
    ~EngineSDL();

    bool isSupported() { return true; } ///< Since SDL is always supported, this function always returns true.

    void drawRectangle(float x, float y, float width, float height, oRGBA color);

	Sprite* createSprite(std::string filename, int index=0) {
		printf("createsprite sdl\n");
		return new SpriteSDL(filename, index);
	}
private:
	friend void SpriteSDL::Blit(float,float,float,float,float,float);

};

#endif
