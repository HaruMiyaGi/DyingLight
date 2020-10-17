#pragma once

float view_x(float x, float width)
{
	return (x) / width * 2.0f - 1.0f;
}


float view_y(float y, float height)
{
	return 1.0f - (y) / height * 2.0f;
}


struct Matrix4x4
{
	float f[4 * 4];
};

struct Vec2
{
	float x, y;

	Vec2(float x, float y)
		: x(x), y(y)
	{}

	Vec2()
		: x(0.0f), y(0.0f)
	{}
};

struct Vec3
{
	float x, y, z;

	Vec3(float x, float y, float z)
		: x(x), y(y), z(z)
	{}

	Vec3()
		: x(0.0f), y(0.0f), z(0.0f)
	{}
};

struct Vec4
{
	float x, y, z, w;

	Vec4(float x, float y, float z, float w)
		: x(x), y(y), z(z), w(w)
	{}

	Vec4()
		: x(0.0f), y(0.0f), z(0.0f), w(0.0f)
	{}
};