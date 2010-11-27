
#include <vector>

#include <boost/shared_ptr.hpp>
#include <boost/date_time.hpp>

#include <cinder/gl/gl.h>
#include <cinder/gl/Texture.h>
#include <cinder/app/AppBasic.h>
#include <cinder/Rand.h>
#include <cinder/CinderMath.h>

class Particle;

// soon to be std::shared_ptr
typedef boost::shared_ptr<Particle> ParticlePtr;

class Diffusion : public ci::app::AppBasic {
public:
    void prepareSettings(Settings* settings);
    void setup();
    void update();
    void draw();
    void shutdown();

    std::vector<bool> field;

private:
    int particleCount;
    std::vector<ParticlePtr> particles;
    ci::Surface surface;
    ci::gl::Texture texture;
    GLubyte* data;
    int dataSize;
};

class Particle {
public:
    Particle(Diffusion& diffusionApp) : field(diffusionApp.field) {
        stuck = false;
        width = diffusionApp.getWindowWidth();
        height = diffusionApp.getWindowHeight();

        boost::posix_time::ptime epoch(boost::gregorian::date(1970,1,1));
        boost::posix_time::ptime now(boost::posix_time::second_clock::universal_time());
        unsigned int seed = (now - epoch).total_seconds();
        
        rand.seed(seed);

        reset();
    }

    void reset() {
        // keep choosing random spots until an empty one is found
        do {
            x = rand.randInt(width);
            y = rand.randInt(height);
        } while (field[y * width + x]);
    }

    void update() {
        // move around
        if (!stuck) {
            // get random int [-1, 1] (hence 2)
            x += rand.randInt(-1, 2);
            y += rand.randInt(-1, 2);
      
            if (x < 0 || y < 0 || x >= width || y >= height) {
                reset();
                return; 
            }

            // test if something is next to us
            if (!alone()) {
                stuck = true;
                field[y * width + x] = true;        
            }
        }
    }

    // returns true if no neighboring pixels
    bool alone() {
        int cx = x;
        int cy = y;

        // get positions
        int lx = cx-1;
        int rx = cx+1;
        int ty = cy-1;
        int by = cy+1;

        if (cx <= 0 || cx >= width || 
            lx <= 0 || lx >= width || 
            rx <= 0 || rx >= width || 
            cy <= 0 || cy >= height || 
            ty <= 0 || ty >= height || 
            by <= 0 || by >= height) return true;

        // pre multiply the ys
        cy *= width;
        by *= width;
        ty *= width;
    
        // N, W, E, S
        if (field[cx + ty] || 
            field[lx + cy] ||
            field[rx + cy] ||
            field[cx + by]) return false;
    
        // NW, NE, SW, SE
        if (field[lx + ty] || 
            field[lx + by] ||
            field[rx + ty] ||
            field[rx + by]) return false;
    
        return true;
    } 

    bool stuck;
    int x, y;

private:
    int width, height;
    std::vector<bool>& field;
    ci::Rand rand;
};


void Diffusion::prepareSettings(Settings* settings) {
    settings->setWindowSize(400, 400);
}

void Diffusion::setup() {
    // this number might need to be smaller for some computers
    particleCount = 20000;
    particles.resize(particleCount);

    // create an array that stores the position of our particles and set them to false
    field.resize(getWindowWidth() * getWindowHeight());

    for (std::vector<bool>::iterator it = field.begin(); it != field.end(); ++it) {
        *it = false;
    }

    // add seed in the center
    int fcenterX = getWindowWidth() / 2;
    int fcenterY = getWindowHeight() / 2;
    field[fcenterX + fcenterY * getWindowWidth()] = true;

    // make particles
    for (int i = 0; i < particles.size(); ++i) {
        particles[i] = ParticlePtr(new Particle(*this));
    }

    dataSize = getWindowWidth() * getWindowHeight();
    data = new GLubyte[getWindowHeight() * getWindowWidth() * 4];

    // set all pixels to white
    for (int i = 0; i < getWindowWidth(); i++) {
        for (int j = 0; j < getWindowHeight(); j++) {
            data[i * getWindowHeight() * 4 + j * 4] = (GLubyte) 255;
            data[i * getWindowHeight() * 4 + j * 4 + 1] = (GLubyte) 255;
            data[i * getWindowHeight() * 4 + j * 4 + 2] = (GLubyte) 255;
            data[i * getWindowHeight() * 4 + j * 4 + 3] = (GLubyte) 255;
        }
    }

    texture = ci::gl::Texture(data, GL_RGBA, getWindowWidth(), getWindowHeight());
}

void Diffusion::update() {
    for(int i=0; i < particleCount; i++) {
        particles[i]->update();
        if (particles[i]->stuck) {
            data[particles[i]->x * getWindowHeight() * 4 + particles[i]->y * 4] = (GLubyte) 0;
            data[particles[i]->x * getWindowHeight() * 4 + particles[i]->y * 4 + 1] = (GLubyte) 0;
            data[particles[i]->x * getWindowHeight() * 4 + particles[i]->y * 4 + 2] = (GLubyte) 0;
            data[particles[i]->x * getWindowHeight() * 4 + particles[i]->y * 4 + 3] = (GLubyte) 255;
        }
    }

    texture = ci::gl::Texture(data, GL_RGBA, getWindowWidth(), getWindowHeight());
}

void Diffusion::draw() {
    ci::gl::draw(texture, ci::Vec2f(0, 0));
}

void Diffusion::shutdown() {
    delete [] data;
}

CINDER_APP_BASIC(Diffusion, ci::app::RendererGl)
