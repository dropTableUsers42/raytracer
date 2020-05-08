#ifndef _OBJECT_H
#define _OBJECT_H

class Object
{
public:
	virtual void init() = 0;
	virtual void render() = 0;
	virtual unsigned int getNumVertices() = 0;
};

#endif