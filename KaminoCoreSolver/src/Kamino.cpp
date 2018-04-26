# include "../include/Kamino.h"

Kamino::Kamino(fReal radius, size_t nTheta, fReal particleDensity,
        float dt, float DT, int frames,
        std::string testPath, std::string particlePath,
        std::string densityImage, std::string solidImage) :
        radius(radius), nTheta(nTheta), nPhi(2 * nTheta), gridLen(M_PI / nTheta),
        particleDensity(particleDensity),
        dt(dt), DT(DT), frames(frames),
        testPath(testPath), particlePath(particlePath), densityImage(densityImage), solidImage(solidImage)
{
    fReal A1 = -1.0; fReal B1 = 0.5; fReal C1 = 0.5; fReal D1 = -0.9; fReal E1 = 1.0;
    fReal A2 = 1.0; fReal B2 = -0.3; fReal C2 = -0.7; fReal D2 = 0.8; fReal E2 = -0.8;
    fPhiCoeff = {A1, B1, C1, D1, E1};
    gThetaCoeff = {0.0};
    lPhiCoeff = {0.0};
    mThetaCoeff = {A2, B2, C2, D2, E2};

    // temporary
    densityImage = "images/flower.jpg";
    solidImage = "images/scriptK.jpg";
}

Kamino::~Kamino()
{
}

void Kamino::run()
{
    KaminoSolver solver(nPhi, nTheta, radius, gridLen, dt, fPhiCoeff, mThetaCoeff);
    KaminoQuantity* d = solver.getAttributeNamed("density");
    initializeDensity(d);
    gridType* g = solver.getGridTypeHandle();
    defineCellTypes(g);
   
    KaminoParticles particles(particleDensity, radius, &solver);
    KaminoQuantity* u = solver.getAttributeNamed("u");
    KaminoQuantity* v = solver.getAttributeNamed("v");

    particles.write_data_bgeo(particlePath, 0);

    float T = 0.0;              // simulation time
    for(int i = 1; i <= frames; i++){
        while(T < i*DT){
            solver.stepForward(dt);
            particles.updatePositions(u, v, dt);
            T += dt;
        }
        solver.stepForward(dt + i*DT - T);
        particles.updatePositions(u, v, dt);
        T = i*DT;

        particles.write_data_bgeo(particlePath, i);
    }
}

void Kamino::test()
{
    KaminoSolver solver(nPhi, nTheta, radius, gridLen, dt, fPhiCoeff, mThetaCoeff);
    KaminoQuantity *d = solver.getAttributeNamed("density");
    initializeDensity(d);
    gridType* g = solver.getGridTypeHandle();
    defineCellTypes(g);

    solver.write_data_bgeo(testPath, 0);

    float T = 0.0;  // simulation time
    for(int i = 1; i <= 6; i++){
        while(T < i*DT){
            solver.stepForward(dt);
            T += dt;
        }
        solver.stepForward(dt + i*DT - T);
        T = i*DT;
        solver.write_data_bgeo(testPath, i);
    }
}

void Kamino::initializeDensity(KaminoQuantity* d)
{
    // read in image
    // Mat image_in;
    // image_in = imread(densityImage, IMREAD_COLOR);
    // if(!image_in.data)
    // {
    //     std::cout << "error: no image data" << std::endl;
    //     exit(1);
    // }

    // // convert to greyscale
    // Mat image_gray; 
    // cvtColor(image_in, image_gray, COLOR_BGR2GRAY);

    // // resize to Nphi x Ntheta
    // Mat image_sized;
    // Size size(nPhi, nTheta);
    // resize(image_gray, image_sized, size);

    // for(size_t i = 0; i < nPhi; ++i)
    // {
    //     for(size_t j = 0; j < nTheta; ++j)
    //     {
    //         Scalar intensity = image_sized.at<uchar>(Point(i,j));
    //         fReal scale = static_cast <fReal> (intensity.val[0]) / 255.0;
    //         d->setValueAt(i, j, scale);
    //     }
    // }
}

void Kamino::defineCellTypes(gridType* g)
{
    for (size_t gPhi = 0; gPhi < nPhi; ++gPhi)
    {
        for (size_t gTheta = 0; gTheta < nTheta; ++gTheta)
        {
            *(g + getIndex(gPhi, gTheta)) = FLUIDGRID;
        }
    }

    // // read in image
    // Mat image_in;
    // image_in = imread(solidImage, IMREAD_COLOR);
    // if(!image_in.data)
    // {
    //     std::cout << "error: no image data" << std::endl;
    //     exit(1);
    // }

    // // convert to greyscale
    // Mat image_gray; 
    // cvtColor(image_in, image_gray, COLOR_BGR2GRAY);

    // // resize to Nphi x Ntheta
    // Mat image_sized;
    // Size size(nPhi, nTheta);
    // resize(image_gray, image_sized, size);

    // // define SOLID cells beneath some threshold
    // for(size_t i = 0; i < nPhi; ++i)
    // {
    //     for(size_t j = 0; j < nTheta; ++j)
    //     {
    //         Scalar intensity = image_sized.at<uchar>(Point(i,j));
    //         if(intensity.val[0] < 128){
    //             *(g + getIndex(i, j)) = SOLIDGRID;
    //         }
    //     }
    // }
}

size_t Kamino::getIndex(size_t x, size_t y)
{
    return y * nPhi + x;
}