#include "stdafx.h"
#include "Bullet.h"

using namespace sf;

Bullet::Bullet() {
	m_BulletShape.setSize(Vector2f(2, 2));
}

void Bullet::stop() {
	m_InFlight = false;
}

bool Bullet::isInFlight() {
	return m_InFlight;
}

void Bullet::shoot(float startX, float startY, float targetX, float targetY) {
	m_InFlight = true;
	m_Position.x = startX;
	m_Position.y = startY;

	float gradient = (startX - targetX) / (startY - targetY);

	if (gradient < 0) {
		gradient *= -1;
	}

	float ratioXY = m_BulletSpeed / (1 + gradient);

	m_BulletDistanceY = ratioXY;
	m_BulletDistanceX = ratioXY * gradient;

	if (targetY < startY) {
		m_BulletDistanceY *= -1;
	}
	if (targetX < startX) {
		m_BulletDistanceX *= -1;
	}

	float range = 1000;
	m_MaxX = startX + range;
	m_MinX = startX - range;
	m_MaxY = startY + range;
	m_MinY = startY - range;

	m_BulletShape.setPosition(m_Position);
}

FloatRect Bullet::getPosition() {
	return m_BulletShape.getGlobalBounds();
}

RectangleShape Bullet::getShape() {
	return m_BulletShape;
}

void Bullet::update(float elapsedTime) {
	m_Position.x += (m_BulletDistanceX * elapsedTime);
	m_Position.y += (m_BulletDistanceY * elapsedTime);

	m_BulletShape.setPosition(m_Position);

	if (m_Position.x < m_MinX || m_Position.x > m_MaxX ||
		m_Position.y < m_MinY || m_Position.y > m_MaxY) {
		m_InFlight = false;
	}
}