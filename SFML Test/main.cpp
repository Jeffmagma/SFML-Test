#define _USE_MATH_DEFINES

#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>

#include <iostream>
#include <vector>
#include <thread>
#include <memory>
#include <string>
#include <sstream>
#include <cmath>

#include "entity.h"

#define PORT 42069
using namespace std;

bool is_arrow(sf::Keyboard::Key key) {
	// Left is 71, Down is 74
	return key >= sf::Keyboard::Left && key <= sf::Keyboard::Down;
}

template <class t>
ostream& operator<<(ostream& o, sf::Vector2<t> v) {
	return o << '[' << v.x << ", " << v.y << ']';
}

void start_server() {
	// Create a listener for connections
	sf::TcpListener listener;
	// If we already have a server here, return
	if (listener.listen(PORT) != sf::Socket::Done) return;
	// Be able to accept clients and data
	listener.setBlocking(false);
	// The list of clients
	vector<client> clients;
	// Keep listening for new connections and data
	while (true) {
		client player;
		// If you received a new player
		if (listener.accept(*player.socket) == sf::Socket::Done) {
			cout << "connected\n";
			// Send him his ID
			sf::Packet packet;
			int id = static_cast<int>(clients.size());
			packet << id;
			player.socket->send(packet);
			// Send everyone else data that a new player connected
			packet << 0 << 0;
			for (client p : clients) {
				p.socket->send(packet);
			}
			// Add him to the list of players
			player.socket->setBlocking(false);
			clients.push_back(player);
		}
		for (auto player : clients) {
			sf::Packet packet;
			if (player.socket->receive(packet) == sf::Socket::Done) {
				int id;
				packet >> id;
				packet >> clients[id];
				cout << "player " << id << " is now at " << clients[id].position << " facing: " << clients[id].direction << "\n";
				for (client p : clients) {
					p.socket->send(packet);
				}
			}
		}
	}
}

int main() {
	// Create the window
	sf::RenderWindow window(sf::VideoMode(800, 600), "Networking Test");
	// Create a server if possible
	thread server(start_server);
	server.detach();
	// Create a connection to the server
	client you;
	you.socket->connect("192.168.1.48", PORT);
	// Get your ID from the server
	sf::Packet packet;
	int id;
	you.socket->receive(packet);
	packet >> id;
	cout << id;

	you.socket->setBlocking(false);
	vector<player> players;
	vector<bullet> bullets;
	bool pressed = false;
	unsigned long long tick = 0;
	// Keep showing the window and checking for events
	while (window.isOpen()) {
		// Handle events
		sf::Event event;
		while (window.pollEvent(event)) {
			switch (event.type) {
			// If the window was closed, close the window
			case sf::Event::Closed: window.close(); break;
			case sf::Event::KeyPressed:
				// If an arrow key was pressed, move the character
				switch (event.key.code) {
				case sf::Keyboard::Left: you.position.x--; break;
				case sf::Keyboard::Right: you.position.x++; break;
				case sf::Keyboard::Up: you.position.y--; break;
				case sf::Keyboard::Down: you.position.y++; break;
				}
			// Fallthrough because your direction changes when the character moves relative to the mouse
			case sf::Event::MouseMoved:
				// Calculate the character's direction
			{
				sf::Vector2i pos = sf::Mouse::getPosition(window);
				double dir = (atan2(you.position.y - pos.y, you.position.x - pos.x)) * 180 / M_PI;
				you.direction = 180 + dir;
				// Send it to the server
				packet.clear();
				packet << id << you;
				you.socket->send(packet); }
				break;
			case sf::Event::MouseButtonPressed:
				pressed = true;
				break;
			case sf::Event::MouseButtonReleased:
				pressed = false;
				break;
			}
		}
		tick++;
		if (pressed && tick % 20 == 0) bullets.push_back(bullet(you.position, you.direction));
		// If you receive data from the server
		if (you.socket->receive(packet) == sf::Socket::Done) {
			// Get the ID of the updated player
			int id;
			packet >> id;
			// If it is a new player, make room for him
			if (players.size() <= id) players.resize(id + 1);
			// Store his data
			packet >> players[id];
		}
		window.clear();
		for (player p : players) {
			// Draw all the players
			sf::RectangleShape r(sf::Vector2f(20, 10));
			r.setOrigin(10, 5);
			r.setFillColor(sf::Color::Red);
			r.setPosition(sf::Vector2f(p.position));
			r.setRotation(p.direction);
			window.draw(r);
		}
		for (int i = 0; i < bullets.size();) {
			bullet& b = bullets[i];
			b.tick();
			if (b.position.x < 0 || b.position.y < 0 || b.position.x > 800 || b.position.y > 600) bullets.erase(bullets.begin() + i);
			else {
				sf::CircleShape c(2);
				c.setOrigin(1, 1);
				c.setFillColor(sf::Color::Yellow);
				c.setPosition(b.position);
				window.draw(c);
				i++;
			}
		}
		window.display();
	}
}