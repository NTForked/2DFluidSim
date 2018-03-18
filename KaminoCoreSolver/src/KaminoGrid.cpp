# include "../include/KaminoSolver.h"

KaminoGrid::KaminoGrid(size_t nx, size_t ny, fReal gridLength, fReal frameDuration) :
	nx(nx), ny(ny), gridLen(gridLength), frameDuration(frameDuration),
	timeStep(0.0), timeElapsed(0.0)
{
	addUAttr("u");		// u velocity
	addVAttr("v");		// v velocity
	addCenteredAttr("rho");	// rho density

	initialize_velocity();
}

KaminoGrid::~KaminoGrid()
{
	for (auto& attr : this->attributeTable)
	{
		delete attr.second;
	}
}

void KaminoGrid::stepForward(fReal timeStep)
{
	this->timeStep = timeStep;
	// TODO
	advection();
	// bodyForce();
	// projection();
}

void KaminoGrid::addCenteredAttr(std::string name)
{
	KaminoAttribute* ptr = new KaminoCenteredAttr(name, this->nx, this->ny, this->gridLen);
	this->attributeTable.emplace(std::pair<std::string, KaminoAttribute*>(name, ptr));
}

void KaminoGrid::addUAttr(std::string name)
{
	KaminoAttribute* ptr = new KaminoUAttr(name, this->nx, this->ny, this->gridLen);
	this->attributeTable.emplace(std::pair<std::string, KaminoAttribute*>(name, ptr));
}

void KaminoGrid::addVAttr(std::string name)
{
	KaminoAttribute* ptr = new KaminoVAttr(name, this->nx, this->ny, this->gridLen);
	this->attributeTable.emplace(std::pair<std::string, KaminoAttribute*>(name, ptr));
}

KaminoAttribute* KaminoGrid::getAttributeNamed(std::string name)
{
	return (*this)[name];
}

KaminoAttribute* KaminoGrid::operator[](std::string name)
{
	return this->attributeTable.at(name);
}

fReal KaminoGrid::FBM(const fReal x, const fReal y){
    fReal total = 0.0f;
    fReal resolution = 10.f;
    fReal persistance = 0.5;
    int octaves = 4;

    for(int i = 0; i < octaves; i++){
        fReal freq = std::pow(2.0f, i);
        fReal amp = std::pow(persistance, i);
        total += amp * interpNoise2D(x * freq / resolution, y * freq / resolution);
    }
    fReal a = 1 - persistance;  // normalization

    return a * total / 2.0f;  // normalized, pseudorandom number between -1 and 1

}

fReal KaminoGrid::interpNoise2D(const fReal x, const fReal y) const{
    fReal intX = std::floor(x);
    fReal fractX = x - intX;
    fReal intY = std::floor(y);
    fReal fractY = y - intY;

    fReal v1 = rand(Eigen::Matrix<fReal, 2, 1>(intX, intY));
    fReal v2 = rand(Eigen::Matrix<fReal, 2, 1>(intX + 1, intY));
    fReal v3 = rand(Eigen::Matrix<fReal, 2, 1>(intX, intY + 1));
    fReal v4 = rand(Eigen::Matrix<fReal, 2, 1>(intX + 1, intY + 1));

    // interpolate for smooth transitions
    fReal i1 = KaminoLerp(v1, v2, fractX);
    fReal i2 = KaminoLerp(v3, v4, fractX);
    return KaminoLerp(i1, i2, fractY);
}

fReal KaminoGrid::rand(const Eigen::Matrix<fReal, 2, 1> vecA) const{
    // return pseudorandom number between -1 and 1
    Eigen::Matrix<fReal, 2, 1> vecB = Eigen::Matrix<fReal, 2, 1>(12.9898, 4.1414);
	fReal val = sin(vecA.dot(vecB) * 43758.5453);
	return val - std::floor(val);
}

void KaminoGrid::write_data_bgeo(const std::string& s, const int frame)
{
# ifndef _MSC_VER
    std::string file = s + std::to_string(frame) + ".bgeo";

    // TODO: interpolate velocities to grid centers and combine into vec2

    Partio::ParticlesDataMutable* parts = Partio::create();
    Partio::ParticleAttribute pH, vH;
    pH = parts->addAttribute("position", Partio::VECTOR, 3);
    vH = parts->addAttribute("v", Partio::VECTOR, 3);

    Eigen::Matrix<float, 3, 1> pos;
    Eigen::Matrix<float, 3, 1> vel;
    fReal velX, velY;

    for(size_t i = 0; i < nx; ++i){
        for(size_t j = 0; j < ny; ++j){
        	velX = (attributeTable["u"]->getValueAt(i, j) + attributeTable["u"]->getValueAt(i + 1, j)) / 2.0;
        	velY = (attributeTable["v"]->getValueAt(i, j) + attributeTable["v"]->getValueAt(i, j + 1)) / 2.0;
        	pos = Eigen::Matrix<float, 3, 1>(i * gridLen, j * gridLen, 0.0);
        	vel = Eigen::Matrix<float, 3, 1>(velX, velY, 0.0);
            int idx = parts->addParticle();
            float* p = parts->dataWrite<float>(pH, idx);
            float* v = parts->dataWrite<float>(vH, idx);
            for (int k = 0; k < 3; ++k){
                p[k] = pos(k, 0);
                v[k] = vel(k, 0);
            }
        }
    }
    Partio::write(file.c_str(), *parts);
    parts->release();
# endif
}

void KaminoGrid::initialize_velocity()
{
	// initialize u velocities
	fReal x = -gridLen / 2.0;
	fReal y = 0.0;
	fReal val = 0.0;

    for(size_t i = 0; i < nx; ++i){
        for(size_t j = 0; j < ny; ++j){
        	val = FBM(sin(2*M_PI*x / (nx*gridLen)), sin(2*M_PI*y / (ny*gridLen)));
            attributeTable["u"]->setValueAt(i, j, val);
            y += gridLen;
        }
        x += gridLen;
    }
    // initialize v velocities
    x = 0.0;
    y = -gridLen / 2.0;

    for(size_t i = 0; i < nx; ++i){
        for(size_t j = 0; j < ny + 1; ++j){
        	val = FBM(std::sin(2.0 * M_PI *x / (nx * gridLen)), sin(2.0 * M_PI * y / (ny * gridLen)));
            attributeTable["v"]->setValueAt(i, j, val);
            y += gridLen;
        }
        x += gridLen;
    }
}

void KaminoGrid::advection()
{
	for (auto quantity : this->attributeTable)
	{
		KaminoAttribute* attr = quantity.second;
		for (size_t gridX = 0; gridX < this->nx; ++gridX)
		{
			for (size_t gridY = 0; gridY < this->ny; ++gridY)
			{
				fReal gX = static_cast<fReal>(gridX * this->gridLen);
				fReal gY = static_cast<fReal>(gridY * this->gridLen);

				fReal uG = (*this)["u"]->sampleAt(gX, gY);
				fReal vG = (*this)["v"]->sampleAt(gX, gY);

				fReal midX = gX - 0.5 * timeStep * uG;
				fReal midY = gY - 0.5 * timeStep * vG;

				fReal uMid = (*this)["u"]->sampleAt(midX, midY);
				fReal vMid = (*this)["v"]->sampleAt(midX, midY);

				fReal pX = gX - timeStep * uMid;
				fReal pY = gY - timeStep * vMid;

				fReal advectedVal = attr->sampleAt(pX, pY);
				attr->writeValueTo(gridX, gridY, advectedVal);
			}
		}
	}
}

void KaminoGrid::swapAttrBuffers()
{

}