#include "stdafx.h"
#include <sstream>
#include <fstream>
#include <SFML/Graphics.hpp>
#include <SFML\Audio.hpp>
#include "ZombieArena.h"
#include "Player.h"
#include "TextureHolder.h"
#include "Bullet.h"
#include "Pickup.h"

using namespace sf;
using namespace std;

int main() {
	/*
	**** Definitions ****
	*/
	TextureHolder textureHilder;
	enum class State {
		PAUSED, LEVELING_UP, GAME_OVER, PLAYING
	};
	State state = State::GAME_OVER;
	State previousState;

	Vector2f resolution;
	resolution.x = VideoMode::getDesktopMode().width;
	resolution.y = VideoMode::getDesktopMode().height;

	RenderWindow window(VideoMode(resolution.x, resolution.y), "Zombie Arena", Style::None);
	View mainView(FloatRect(0, 0, resolution.x, resolution.y));

	Clock clock;
	Time gameTimeTotal;

	Vector2f mouseWorldPosition;
	Vector2i mouseScreenPosition;

	Player player;

	IntRect arena;

	VertexArray background;
	Texture textureBackground = TextureHolder::GetTexture("graphics/background_sheet.png");

	int numZombies;
	int numZombiesAlive;
	Zombie* zombies = nullptr;

	Bullet bullets[100];
	int currentBullet = 0;

	const int START_BULLETS_SPARE = 24;
	const int START_CLIP_SIZE = 6;

	int clipSize = START_CLIP_SIZE;
	int bulletsSpare = START_BULLETS_SPARE;
	int bulletsInClip = START_CLIP_SIZE;
	float fireRate = 1;

	Time lastPressed;

	window.setMouseCursorVisible(false);
	Sprite spriteCrosshair = Sprite(TextureHolder::GetTexture("graphics/crosshair.png"));
	spriteCrosshair.setOrigin(spriteCrosshair.getTexture()->getSize().x / 2, spriteCrosshair.getTexture()->getSize().y / 2);

	Pickup healthPickup(1);
	Pickup ammoPickup(2);

	int score = 0;
	int highScore = 0;

	Sprite gameOver = Sprite(TextureHolder::GetTexture("graphics/background.png"));
	gameOver.setPosition(0, 0);
	gameOver.setScale((resolution.x / gameOver.getTexture()->getSize().x), (resolution.y / gameOver.getTexture()->getSize().y));

	View hudView(FloatRect(0, 0, resolution.x, resolution.y));

	Sprite ammoIcon = Sprite(TextureHolder::GetTexture("graphics/ammo_icon.png"));
	ammoIcon.setOrigin(0, ammoIcon.getTexture()->getSize().y);
	ammoIcon.setPosition(28, resolution.y);

	Font font;
	font.loadFromFile("fonts/zombiecontrol.ttf");

	int boundary = 30;

	Text pausedText;
	pausedText.setFont(font);
	pausedText.setCharacterSize(85);
	pausedText.setFillColor(Color::White);
	pausedText.setString("Press Enter \nto Continue");
	pausedText.setOrigin(pausedText.getGlobalBounds().width / 2, pausedText.getGlobalBounds().height / 2);
	pausedText.setPosition(resolution.x / 2, resolution.y / 2);

	Text gameOverText;
	gameOverText.setFont(font);
	gameOverText.setCharacterSize(80);
	gameOverText.setFillColor(Color::White);
	gameOverText.setString("Press Enter to Play");
	gameOverText.setOrigin(gameOverText.getGlobalBounds().width / 2, gameOverText.getGlobalBounds().height / 2);
	gameOverText.setPosition(resolution.x / 2, resolution.y / 2);

	Text levelUpText;
	levelUpText.setFont(font);
	levelUpText.setCharacterSize(80);
	levelUpText.setFillColor(Color::White);
	stringstream levelUpStream;
	levelUpStream <<
		"1- Increased rate of fire" <<
		"\n2- Increased clip size (next reload)" <<
		"\n3- Increased max health" <<
		"\n4- Increased run speed" <<
		"\n5- More and better health pickups" <<
		"\n6- More and better ammo pickups";
	levelUpText.setString(levelUpStream.str());
	levelUpText.setOrigin(levelUpText.getGlobalBounds().width / 2, levelUpText.getGlobalBounds().height / 2);
	levelUpText.setPosition(resolution.x / 2, resolution.y / 2);

	Text ammoText;
	ammoText.setFont(font);
	ammoText.setCharacterSize(55);
	ammoText.setFillColor(Color::White);
	stringstream ssA;
	ssA << bulletsInClip << "/" << bulletsSpare;
	ammoText.setString(ssA.str());
	ammoText.setOrigin(ammoText.getGlobalBounds().left, ammoText.getGlobalBounds().height);
	ammoText.setPosition(ammoIcon.getPosition().x + ammoIcon.getGlobalBounds().width + boundary, resolution.y);

	Text scoreText;
	scoreText.setFont(font);
	scoreText.setCharacterSize(55);
	scoreText.setFillColor(Color::White);
	scoreText.setString("Score:    ");
	scoreText.setPosition(boundary, boundary);

	ifstream inputFile("gamedata/scores.txt");
	if (inputFile.is_open()){
		inputFile >> highScore;
		inputFile.close();
	}

	Text highScoreText;
	highScoreText.setFont(font);
	highScoreText.setCharacterSize(50);
	highScoreText.setFillColor(Color::White);
	highScoreText.setString("High Score:    ");
	highScoreText.setPosition(boundary, scoreText.getGlobalBounds().height + boundary);

	Text zombiesText;
	zombiesText.setFont(font);
	zombiesText.setCharacterSize(50);
	zombiesText.setFillColor(Color::White);
	zombiesText.setString("     Zombies Remaining");
	zombiesText.setOrigin(zombiesText.getGlobalBounds().width, zombiesText.getGlobalBounds().height);
	zombiesText.setPosition(resolution.x, resolution.y);

	int wave = 1;
	Text waveText;
	waveText.setFont(font);
	waveText.setFillColor(Color::White);
	waveText.setString("Wave:     ");
	waveText.setCharacterSize(50);
	waveText.setOrigin(waveText.getGlobalBounds().width, waveText.getGlobalBounds().top);
	waveText.setPosition(boundary, resolution.y);

	RectangleShape healthBar;
	healthBar.setFillColor(Color::Red);
	healthBar.setSize(Vector2f(player.getHealth() * 3, 70));
	healthBar.setOrigin(healthBar.getSize().x / 2, healthBar.getSize().y);
	healthBar.setPosition(resolution.x / 2, boundary);

	int framesSinceLastHUDUpdate = 0;
	int fpsMeasurementFrameInterval = 1000;

	SoundBuffer hitBuffer;
	SoundBuffer splatBuffer;
	SoundBuffer shootBuffer;
	SoundBuffer reloadBuffer;
	SoundBuffer pickUpBuffer;
	SoundBuffer powerUpBuffer;
	hitBuffer.loadFromFile("sound/hit.wav");
	splatBuffer.loadFromFile("sound/splat.wav");
	shootBuffer.loadFromFile("sound/shoot.wav");
	reloadBuffer.loadFromFile("sound/reload.wav");
	pickUpBuffer.loadFromFile("sound/pickup.wav");
	powerUpBuffer.loadFromFile("sound/powerup.wav");

	Sound hit;
	Sound splat;
	Sound shoot;
	Sound reload;
	Sound pickUp;
	Sound powerUp;
	hit.setBuffer(hitBuffer);
	splat.setBuffer(splatBuffer);
	shoot.setBuffer(shootBuffer);
	reload.setBuffer(reloadBuffer);
	pickUp.setBuffer(pickUpBuffer);
	powerUp.setBuffer(powerUpBuffer);

	/*
	**** Main Game Loop ****
	*/
	while (window.isOpen()) {
		/*
		**** Handle Events ****
		*/

		Event event;
		while (window.pollEvent(event)) {
			if (event.type == Event::KeyPressed) {
				if (event.key.code == Keyboard::Return && state == State::PLAYING) {
					state = State::PAUSED;
				}
				else if (event.key.code == Keyboard::Return && state == State::PAUSED) {
					state = State::PLAYING;
					clock.restart();
				}
				else if (event.key.code == Keyboard::Return && state == State::GAME_OVER) {
					state = State::PLAYING;

					arena.width = 500;
					arena.height = 500;
					arena.top = 0;
					arena.left = 0;

					int tileSize = createBackground(background, arena);
					player.spawn(arena, resolution, tileSize);

					bulletsInClip = START_CLIP_SIZE;
					bulletsSpare = START_BULLETS_SPARE;
					clipSize = START_CLIP_SIZE;
					fireRate = 1;
					currentBullet = 0;
					score = 0;
					wave = 1;
					player.resetPlayerStats();

					healthPickup.setArena(arena);
					ammoPickup.setArena(arena);

					numZombies = 5;

					delete[] zombies;
					zombies = createHorde(numZombies, arena);
					numZombiesAlive = numZombies;

					clock.restart();
				}

				if (state == State::PLAYING) {
					if (event.key.code == Keyboard::R) {
						int refillSize = clipSize - bulletsInClip;
						if (bulletsSpare >= refillSize) {
							bulletsSpare -= refillSize;
							bulletsInClip = clipSize;
						}
						else if (bulletsSpare >= 0) {
							bulletsInClip += bulletsSpare;
							bulletsSpare = 0;
						}
						reload.play();
					}
				}
			}
			else if (event.type == Event::LostFocus && state == State::PLAYING) {
				state = State::PAUSED;
			}
			else if (event.type == Event::Closed) {
				window.close();
			}
		}
		if (Keyboard::isKeyPressed(Keyboard::Escape)) {
			window.close();
		}

		if (state == State::PLAYING) {
			if (Keyboard::isKeyPressed(Keyboard::W)) {
				player.moveUp();
			}
			else {
				player.stopUp();
			}
			if (Keyboard::isKeyPressed(Keyboard::S)) {
				player.moveDown();
			}
			else {
				player.stopDown();
			}
			if (Keyboard::isKeyPressed(Keyboard::A)) {
				player.moveLeft();
			}
			else {
				player.stopLeft();
			}
			if (Keyboard::isKeyPressed(Keyboard::D)) {
				player.moveRight();
			}
			else {
				player.stopRight();
			}

			if (Mouse::isButtonPressed(Mouse::Left)) {
				if (gameTimeTotal.asSeconds() - lastPressed.asSeconds() > fireRate && bulletsInClip > 0) {
					bullets[currentBullet].shoot(player.getCentre().x, player.getCentre().y, mouseWorldPosition.x, mouseWorldPosition.y);
					shoot.play();
					currentBullet++;
					if (currentBullet > 99) {
						currentBullet = 0;
					}
					lastPressed = gameTimeTotal;
					bulletsInClip--;
				}
			}
		}
		if (state == State::LEVELING_UP) {
			if (event.key.code == Keyboard::Num1) {
				fireRate * .8;
				state = State::PLAYING;
			}
			if (event.key.code == Keyboard::Num2) {
				clipSize += START_CLIP_SIZE;
				state = State::PLAYING;
			}
			if (event.key.code == Keyboard::Num3) {
				player.upgradeHealth();
				state = State::PLAYING;
			}
			if (event.key.code == Keyboard::Num4) {
				player.upgradeSpeed();
				state = State::PLAYING;
			}
			if (event.key.code == Keyboard::Num5) {
				healthPickup.upgrade();
				state = State::PLAYING;
			}
			if (event.key.code == Keyboard::Num6) {
				ammoPickup.upgrade();
				state = State::PLAYING;
			}

			if (state == State::PLAYING) {
				wave++;
				arena.width = 500 * (wave * 1.5);
				arena.height = 500 * (wave * 1.5);

				int tileSize = createBackground(background, arena);
				player.spawn(arena, resolution, tileSize);

				if (bulletsSpare < START_BULLETS_SPARE) {
					bulletsSpare = START_BULLETS_SPARE;
				}

				healthPickup.setArena(arena);
				ammoPickup.setArena(arena);

				numZombies = 5 * (wave * 2);

				delete[] zombies;
				zombies = createHorde(numZombies, arena);
				numZombiesAlive = numZombies;

				powerUp.play();

				clock.restart();
			}
		}
		if (state == State::GAME_OVER) {
			
		}

		/*
		**** Update Frame ****
		*/

		if (state == State::PLAYING) {
			Time dt = clock.restart();
			gameTimeTotal += dt;
			float dtAsSeconds = dt.asSeconds();

			mouseScreenPosition = Mouse::getPosition(window);
			mouseWorldPosition = window.mapPixelToCoords(mouseScreenPosition, mainView);

			spriteCrosshair.setPosition(mouseWorldPosition);

			player.update(dtAsSeconds, mouseScreenPosition);
			Vector2f playerPosition(player.getCentre());

			mainView.setCenter(playerPosition);

			for (int i = 0; i < numZombies; i++) {
				if (zombies[i].isAlive()) {
					zombies[i].update(dtAsSeconds, playerPosition);
				}
			}
			for (int i = 0; i < 100; i++) {
				if (bullets[i].isInFlight()) {
					bullets[i].update(dtAsSeconds);
				}
			}

			healthPickup.update(dtAsSeconds);
			ammoPickup.update(dtAsSeconds);

			for (int i = 0; i < 100; i++) {
				for (int j = 0; j < numZombies; j++) {
					if (bullets[i].isInFlight() && zombies[j].isAlive()) {
						if (bullets[i].getPosition().intersects(zombies[j].getPosition())) {
							bullets[i].stop();
							if (zombies[j].hit()) {
								splat.play();
								score += 10;
								if (score >= highScore) {
									highScore = score;
								}
								numZombiesAlive--;

								if (numZombiesAlive == 0) {
									state = State::LEVELING_UP;
								}
							}
						}
					}
				}
			}

			for (int i = 0; i < numZombies; i++) {
				if (zombies[i].isAlive()) {
					if (player.getPostition().intersects(zombies[i].getPosition())) {
						if (player.hit(gameTimeTotal)) {
							hit.play();
						}
						if (player.getHealth() <= 0) {
							state = State::GAME_OVER;
						}
					}
				}
			}

			if (player.getPostition().intersects(healthPickup.getPosition()) && healthPickup.isSpawned()) {
				player.increaseHealthLevel(healthPickup.gotIt());
				pickUp.play();
			}
			if (player.getPostition().intersects(ammoPickup.getPosition()) && ammoPickup.isSpawned()) {
				bulletsSpare += ammoPickup.gotIt();
				pickUp.play();
			}

			healthBar.setSize(Vector2f(player.getHealth() * 3, 70));

			framesSinceLastHUDUpdate++;

			if (framesSinceLastHUDUpdate > fpsMeasurementFrameInterval) {
				stringstream ssAmmo;
				stringstream ssScore;
				stringstream ssHighScore;
				stringstream ssWave;
				stringstream ssZombies;

				ssAmmo << bulletsInClip << "/" << bulletsSpare;
				ammoText.setString(ssAmmo.str());

				ssScore << "Score: " << score;
				scoreText.setString(ssScore.str());

				ssHighScore << "High Score: " << highScore;
				highScoreText.setString(ssHighScore.str());

				ssWave << "Wave: " << wave;
				waveText.setString(ssWave.str());

				ssZombies << numZombiesAlive << " Zombies Remaining";
				zombiesText.setString(ssZombies.str());

				framesSinceLastHUDUpdate = 0;
			}
		}

		/*
		**** Draw Scene ****
		*/

		if (state == State::PLAYING) {
			window.clear();
			window.setView(mainView);
			window.draw(background, &textureBackground);

			for (int i = 0; i < numZombies; i++) {
				window.draw(zombies[i].getSprite());
			}

			for (int i = 0; i < 100; i++) {
				if (bullets[i].isInFlight()) {
					window.draw(bullets[i].getShape());
				}
			}

			window.draw(player.getSprite());

			if (ammoPickup.isSpawned()) {
				window.draw(ammoPickup.getSprite());
			}
			if (healthPickup.isSpawned()) {
				window.draw(healthPickup.getSprite());
			}

			window.draw(spriteCrosshair);

			window.setView(hudView);
			window.draw(ammoIcon);
			window.draw(ammoText);
			window.draw(scoreText);
			window.draw(highScoreText);
			window.draw(healthBar);
			window.draw(waveText);
			window.draw(zombiesText);
		}
		if (state == State::LEVELING_UP) {
			window.draw(gameOver);
			window.draw(levelUpText);
		}
		if (state == State::PAUSED) {
			window.draw(pausedText);
		}
		if (state == State::GAME_OVER) {
			window.draw(gameOver);
			window.draw(gameOverText);
			window.draw(scoreText);
			window.draw(highScoreText);
		}

		/*
		**** Display Window ****
		*/
		window.display();
	}
	ofstream outputFile("gamedata/scores.txt");
	outputFile << highScore;
	outputFile.close();
	delete[] zombies;
    return 0;
}