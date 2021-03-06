/***************************************************************************
 *   Copyright (C) 1998-2013 by authors (see AUTHORS.txt)                  *
 *                                                                         *
 *   This file is part of LuxRender.                                       *
 *                                                                         *
 *   Lux Renderer is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   Lux Renderer is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 *                                                                         *
 *   This project is based on PBRT ; see http://www.pbrt.org               *
 *   Lux Renderer website : http://www.luxrender.net                       *
 ***************************************************************************/

// queryable.cpp
#include "queryable.h"
#include "context.h"

namespace lux
{

Queryable::Queryable(const Queryable &q) : attributes(q.attributes),
		name(q.name + "-copied-to-" + boost::lexical_cast<string>(this)),
		nullAttribute(q.nullAttribute) {
	// Add this object to the registry
	if (Context::GetActive())
		Context::GetActive()->registry.Insert(this);
	else
		LOG(LUX_ERROR, LUX_NOTSTARTED) << "luxInit() not called";
}

Queryable::Queryable(std::string _name) : name(_name)
{
	// Add this object to the registry
	if (Context::GetActive())
		Context::GetActive()->registry.Insert(this);
	else
		LOG(LUX_ERROR, LUX_NOTSTARTED) << "luxInit() not called";
}

Queryable::~Queryable()
{
	//remove this object from the registry
	if (Context::GetActive())
		Context::GetActive()->registry.Erase(this);
	else
		LOG(LUX_ERROR, LUX_NOTSTARTED) << "luxInit() not called";
}

}//namespace lux
