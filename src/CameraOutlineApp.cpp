#include "cinder/app/AppNative.h"
#include "cinder/Capture.h"
#include "cinder/gl/gl.h"
#include "cinder/ImageIo.h"
#include "cinder/gl/Texture.h"
#include "cinder/ip/EdgeDetect.h"

using namespace ci;
using namespace ci::app;
using namespace std;

const int WIDTH = 640;
const int HEIGHT = 480;
const int NUM_PARTICLES = 10000;

float random01()
{
	return rand() / (float)RAND_MAX;
}

float lerp(float a, float b, float t)
{
	return a + (b - a) * t;
}

class Particle
{
private:
	float _drag;
	float _value;
	float _velocity;
	
public:
	Vec3f Position;

	Particle(Vec3f position, float drag)
	{
		Position = position;

		_drag = drag;
		_velocity = 0;
		_value = 0;
	}

	void update(float value, float dt)
	{
		auto newVelocity = abs(value - _value);
		if (newVelocity > _velocity)
		{
			_velocity = lerp(_velocity, newVelocity, dt);
		}
		else
		{
			_velocity = lerp(_velocity, newVelocity, dt * _drag);
		}
		
		_value = lerp(_value, value, dt * _drag);
	}

	float getValue()
	{
		return _value;
	}

	float getVelocity()
	{
		return _velocity;
	}
};

class CameraOutlineApp : public AppNative
{
public:
	double _last;
	Capture _capture;
	gl::Texture _texture;
	Channel _channel;
	unique_ptr<Particle> _particles[NUM_PARTICLES];
	unique_ptr<Surface> _destinationSurface;
	
	void prepareSettings(Settings *settings);
	void setup();
	void mouseDown(MouseEvent event);
	void update();
	void draw();

	void drawParticle(Particle* particle);

	void updateFrameTexture();
};

void CameraOutlineApp::prepareSettings(Settings *settings)
{
	settings->setFullScreen(true);
	settings->setFrameRate(60.0f);
}

void CameraOutlineApp::setup()
{
	_last = getElapsedSeconds();

	auto devices = Capture::getDevices(true);

	if (devices.size() > 0)
	{
		_capture = *Capture::create(WIDTH, HEIGHT, devices[0]).get();
		_capture.start();

		_texture = gl::Texture(_capture.getSurface());
		_channel = Channel(_texture.getWidth(), _texture.getHeight());
		_destinationSurface = nullptr;

		Vec2f max(
			WIDTH,
			HEIGHT);
		for (auto i = 0; i < NUM_PARTICLES; i++)
		{
			_particles[i] = unique_ptr<Particle>(new Particle(
				Vec3f(
					random01() * max.x,
					random01() * max.y,
					0),
				.5));
		}
	}
	else
	{
		cout << "Could not find any capture devices.";
		exit(1);
	}
}

void CameraOutlineApp::mouseDown(MouseEvent event)
{

}

void CameraOutlineApp::update()
{
	if (_capture.checkNewFrame())
	{
		updateFrameTexture();
	}
}

void CameraOutlineApp::draw()
{
	gl::clear(Color(0, 0, 0), true);

	auto bounds = getWindowBounds();
	auto widthMultiplier = bounds.getWidth() / WIDTH;

	auto scaledWidth = widthMultiplier * WIDTH;
	auto scaledHeight = widthMultiplier * HEIGHT;
	auto x = (int)(bounds.getWidth() / 2.0 - scaledWidth / 2.0);
	auto y = (int)(bounds.getHeight() / 2.0 - scaledHeight / 2.0);

	//gl::draw(
		//_texture,
		//Rectf(x, y, x + scaledWidth, y + scaledHeight));

	if (nullptr == _destinationSurface)
	{
		return;
	}

	auto current = getElapsedSeconds();
	auto dt = 10 * ((float) current - _last);
	_last = current;

	for (int i = 0; i < NUM_PARTICLES; i++)
	{
		auto particle = _particles[i].get();

		auto pixelx = (int) particle->Position.x;
		auto pixely = (int) particle->Position.y;

		auto color = _destinationSurface->getPixel(Vec2i(
			pixelx,
			pixely));
		auto luma = 0.2126 * (color.r / 255.0) + 0.7152 * (color.g / 255.0) + 0.0722 * (color.b / 255.0);
		particle->update(luma, dt);

		gl::color(Color(particle->getValue(), 0.0, 0.0));
		
		gl::drawSolidCircle(
			Vec2f(
				x + pixelx * widthMultiplier,
				y + pixely * widthMultiplier),
				50 * max((float) 0.01, particle->getVelocity()));
	}
}

void CameraOutlineApp::updateFrameTexture()
{
	auto captureWidth = _capture.getWidth();
	auto captureHeight = _capture.getHeight();

	auto surface = _capture.getSurface();

	// the capture bounds may change
	if (captureWidth != _texture.getWidth()
		|| captureHeight != _texture.getHeight())
	{
		_texture = gl::Texture(captureWidth, captureHeight);
		_channel = Channel(captureWidth, captureHeight);
		_destinationSurface = unique_ptr<Surface>(new Surface(surface));
	}

	//ip::edgeDetectSobel(surface, _destinationSurface.get());
	_destinationSurface = unique_ptr<Surface>(new Surface(surface));
	_texture.update(*_destinationSurface.get());
}

CINDER_APP_NATIVE(CameraOutlineApp, RendererGl)