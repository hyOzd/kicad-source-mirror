/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef __SHAPE_INDEX_LIST_H
#define __SHAPE_INDEX_LIST_H

#include <boost/unordered_map.hpp>

template <class T> const SHAPE *defaultShapeFunctor( const T aItem )
{
	return aItem->GetShape();
}

template <class T, const SHAPE *(ShapeFunctor)(const T) = defaultShapeFunctor<T> >

class SHAPE_INDEX_LIST {
	
	struct ShapeEntry {
		ShapeEntry(T aParent)
		{
			shape = ShapeFunctor(aParent);
			bbox = shape->BBox(0);
			parent = aParent;
		}

		~ShapeEntry()
		{
			
		}

		T parent;
		const SHAPE *shape;
		BOX2I bbox;
	};

	typedef std::vector<ShapeEntry> ShapeVec;
	typedef typename std::vector<ShapeEntry>::iterator ShapeVecIter;	
	
public:

// "Normal" iterator interface, for STL algorithms. 
	class iterator {

		public:
			iterator() {};

			iterator( ShapeVecIter aCurrent)
				: m_current(aCurrent) {};

			iterator(const iterator &b) : 
				m_current(b.m_current) {};

			T operator*() const
			{
				return (*m_current).parent;
			}

			void operator++()
			{
				++m_current;
			}

			iterator& operator++(int dummy)
			{
				++m_current;
				return *this;
			}

			bool operator ==( const iterator& rhs ) const
			{
				return m_current == rhs.m_current;
			}

			bool operator !=( const iterator& rhs ) const
			{
				return m_current != rhs.m_current;
			}

			const iterator& operator=(const iterator& rhs) 
			{
				m_current = rhs.m_current;
				return *this;
			}

		private:
			ShapeVecIter m_current;
	};

// "Query" iterator, for iterating over a set of spatially matching shapes.
	class query_iterator {
		public:

			query_iterator()
			{

			}

			query_iterator(  ShapeVecIter aCurrent, ShapeVecIter aEnd, SHAPE *aShape, int aMinDistance, bool aExact)
				: m_end(aEnd),
				  m_current(aCurrent),
				  m_shape(aShape),
				  m_minDistance(aMinDistance),
				  m_exact(aExact)
			{
				if(aShape)
				{
					m_refBBox = aShape->BBox();
					next();
				}
			}

			query_iterator(const query_iterator &b)
				: m_end(b.m_end),
				  m_current(b.m_current),
				  m_shape(b.m_shape),
				  m_minDistance(b.m_minDistance),
				  m_exact(b.m_exact),
				  m_refBBox(b.m_refBBox)
			{

			}

			
			T operator*() const
			{
				return (*m_current).parent;
			}

			query_iterator& operator++()
			{
				++m_current;
				next();
			 	return *this;
			}

			query_iterator& operator++(int dummy)
			{
				++m_current;
				next();
				return *this;
			}

			bool operator ==( const query_iterator& rhs ) const
			{
				return m_current == rhs.m_current;
			}

			bool operator !=( const query_iterator& rhs ) const
			{
				return m_current != rhs.m_current;
			}

			const query_iterator& operator=(const query_iterator& rhs) 
			{
				m_end = rhs.m_end;
				m_current = rhs.m_current;
				m_shape = rhs.m_shape;
				m_minDistance = rhs.m_minDistance;
				m_exact = rhs.m_exact;
				m_refBBox = rhs.m_refBBox;
				return *this;
			}

		private:

			void next()
			{
				while(m_current != m_end)
				{
					if (m_refBBox.Distance(m_current->bbox) <= m_minDistance)
					{
						if(!m_exact || m_current->shape->Collide(m_shape, m_minDistance))
							return;
					}
					++m_current;
				}
			}

			ShapeVecIter m_end;
			ShapeVecIter m_current;
			BOX2I m_refBBox;
			bool m_exact;
			SHAPE *m_shape;
			int m_minDistance;
	};

	void Add(T aItem)
	{
		ShapeEntry s (aItem);

		m_shapes.push_back(s);
	}
	
	void Remove(const T aItem)
	{
		ShapeVecIter i;
		
		for(i=m_shapes.begin(); i!=m_shapes.end();++i)
		{
			if(i->parent == aItem)
				break;
		}

		if(i == m_shapes.end())
			return;

		m_shapes.erase(i);
	}

	int Size() const
	{
		return m_shapes.size();
	}

	template<class Visitor>
		int Query( const SHAPE *aShape, int aMinDistance, Visitor &v, bool aExact = true) //const
		{
			ShapeVecIter i;
			int n = 0;
			VECTOR2I::extended_type minDistSq = (VECTOR2I::extended_type) aMinDistance * aMinDistance;

			BOX2I refBBox = aShape->BBox();

			for(i = m_shapes.begin(); i!=m_shapes.end(); ++i)
			{
				if (refBBox.SquaredDistance(i->bbox) <= minDistSq)
				{
					if(!aExact || i->shape->Collide(aShape, aMinDistance))
					{
						n++;
						if(!v( i->parent ))
							return n;
					}
				}
			}
			return n;
		}

	void Clear()
	{
		m_shapes.clear();
	}
	
	query_iterator qbegin( SHAPE *aShape, int aMinDistance, bool aExact ) 
	{
		return query_iterator( m_shapes.begin(), m_shapes.end(), aShape, aMinDistance, aExact);
	}

	const query_iterator qend() 
	{
			return query_iterator( m_shapes.end(), m_shapes.end(), NULL, 0, false );
	}

	iterator begin()
	{
		return iterator( m_shapes.begin() );
	}

	iterator end()
	{
		return iterator( m_shapes.end() );
	}

private:

	ShapeVec m_shapes;
};

#endif