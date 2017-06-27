#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <SFML/Network/Packet.hpp>

#include <iostream>
#include <vector>
#include <thread>
#include <memory>
#include <string>
#include <sstream>
#include <cmath>

#define PORT 42069
using namespace std;

struct player {
	sf::Vector2i position = sf::Vector2i(0, 0);
	int direction;
};

struct client : player {
	sf::TcpSocket* socket = new sf::TcpSocket;
};

bool is_arrow(sf::Keyboard::Key key) {
	// Left is 71, Down is 74
	return key >= sf::Keyboard::Left && key <= sf::Keyboard::Down;
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
				int id, x, y, dir;
				packet >> id >> x >> y >> dir;
				clients[id].position = sf::Vector2i(x, y);
				clients[id].direction = dir;
				cout << "player " << id << " is now at " << clients[id].position.x << "," << clients[id].position.y << " facing: " << clients[id].direction << "\n";
				packet.clear();
				packet << id << x << y << dir;
				for (client p : clients) {
					p.socket->send(packet);
				}
			}
		}
	}
}

int main() {
	// Create the window
	sf::RenderWindow window(sf::VideoMode(400, 300), "Networking Test");
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
			// Fallthrough because you should be doing this anyway
			case sf::Event::MouseMoved:
				sf::Vector2i pos = sf::Mouse::getPosition(window);
				double dir = (atan2(you.position.y - pos.y, you.position.x - pos.x)) * 180 / 3.141592;
				you.direction = dir;
				packet.clear();
				packet << id << you.position.x << you.position.y << you.direction;
				you.socket->send(packet);
				break;
			}
		}
		// If you receive data from the server
		if (you.socket->receive(packet) == sf::Socket::Done) {
			int id, x, y, dir;
			packet >> id >> x >> y >> dir;
			if (players.size() <= id) players.push_back({ {x, y}, dir });
			else {
				players[id].position = sf::Vector2i(x, y);
				players[id].direction = dir;
			}
		}
		window.clear();
		for (player p : players) {
			sf::RectangleShape r(sf::Vector2f(20, 10));
			r.setOrigin(10, 5);
			r.setFillColor(sf::Color::Red);
			r.setPosition(sf::Vector2f(p.position));
			r.setRotation(p.direction);
			window.draw(r);
		}
		window.display();
	}
}