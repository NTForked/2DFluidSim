# include "../include/KaminoSolver.h"

KaminoUAttr::KaminoUAttr(std::string attributeName, size_t nx, size_t ny, fReal gridLen)
	: KaminoAttribute(attributeName, nx, ny, gridLen)
{
	thisStep = new fReal[nx * ny];
	nextStep = new fReal[nx * ny];
}

KaminoUAttr::~KaminoUAttr(){}

size_t KaminoUAttr::getIndex(size_t x, size_t y)
{
# ifdef DEBUGBUILD
	// Handle exception
# endif
	return x * nx + y;
}

fReal KaminoUAttr::sampleAt(fReal x, fReal y)
{
	fReal xOffset = 0.5;
	fReal yOffset = 0.5;
	x = x + xOffset;
	y = y + yOffset;

	size_t lowerX = getWarpedXIndex(x);
	size_t lowerY = getWarpedYIndex(y);
	size_t upperX = (lowerX + 1) % nx;
	size_t upperY = (lowerY + 1) % ny;

	fReal lowerLeft = getValueAt(lowerX, lowerY);
	fReal upperLeft = getValueAt(lowerX, upperY);
	fReal lowerRight = getValueAt(upperX, lowerY);
	fReal upperRight = getValueAt(upperX, upperY);

	fReal alphaX = x - lowerX;
	fReal alphaY = y - lowerY;

	fReal lerpedLower = KaminoLerp<fReal>(lowerLeft, lowerRight, alphaX);
	fReal lerpedUpper = KaminoLerp<fReal>(upperLeft, upperRight, alphaX);
	fReal lerped = KaminoLerp<fReal>(lerpedLower, lerpedUpper, alphaY);

	return lerped;
}

// x has no offset yet.
size_t KaminoUAttr::getWarpedXIndex(fReal x)
{
	int loops = static_cast<int>(std::floor(x / static_cast<fReal>(this->nx)));
	int flooredX = static_cast<int>(std::floor(x));
	int warpedX = flooredX - loops * static_cast<int>(nx);

	return static_cast<size_t>(warpedX);
}

// y has no offset yet either.
size_t KaminoUAttr::getWarpedYIndex(fReal y)
{
	int loops = static_cast<int>(std::floor(y / static_cast<fReal>(this->ny)));
	int flooredY = static_cast<int>(std::floor(y));
	int warpedY = flooredY - loops * static_cast<int>(ny);

	return static_cast<size_t>(warpedY);
}