#pragma once

#include <SFML/Graphics.hpp>
using namespace sf;

class Player {
private:
	const float START_SPEED = 200;
	const float START_HEALTH = 100;

	Vector2f m_Position;

	Sprite m_Sprite;

	Vector2f m_Resolution;

	IntRect m_Arena;

	int m_TileSize;

	bool m_upPressed;
	bool m_DownPressed;
	bool m_LeftPressed;
	bool m_RightPressed;

	int m_Health;

	int m_MaxHealth;

	Time m_LastHit;

	float m_Speed;

public:
	Player();

	void spawn(IntRect arena, Vector2f resolution, int tileSize);

	void resetPlayerStats();

	Time getLastHitTime();

	bool hit(Time timeHit);

	FloatRect getPostition();

	Vector2f getCentre();

	float getRotation();

	Sprite getSprite();

	void moveLeft();
	void moveRight();
	void moveUp();
	void moveDown();

	void stopLeft();
	void stopRight();
	void stopUp();
	void stopDown();

	void update(float elapsedTime, Vector2i mousePosition);

	void upgradeSpeed();

	void upgradeHealth();

	void increaseHealthLevel(int amount);

	int getHealth();
};