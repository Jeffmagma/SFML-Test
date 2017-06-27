#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <SFML/Network/Packet.hpp>

#include <iostream>
#include <vector>
#include <thread>
#include <memory>
#include <string>
#include <sstream>

#define PORT 42069
using namespace std;

struct player {
	sf::TcpSocket* socket = new sf::TcpSocket;
	sf::Vector2i position = sf::Vector2i(0, 0);
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
	vector<player> clients;
	// Keep listening for new connections and data
	while (true) {
		player client;
		// If you received a new player
		if (listener.accept(*client.socket) == sf::Socket::Done) {
			cout << "connected\n";
			// Send him his ID
			sf::Packet packet;
			int id = static_cast<int>(clients.size());
			packet << id;
			client.socket->send(packet);
			// Send everyone else data that a new player connected
			packet << 0 << 0;
			for (player p : clients) {
				p.socket->send(packet);
			}
			// Add him to the list of players
			client.socket->setBlocking(false);
			clients.push_back(client);
		}
		for (auto client : clients) {
			sf::Packet packet;
			if (client.socket->receive(packet) == sf::Socket::Done) {
				int id, x, y;
				packet >> id >> x >> y;
				clients[id].position = sf::Vector2i(x, y);
				cout << "player " << id << " is now at " << clients[id].position.x << "," << clients[id].position.y << "\n";
				packet.clear();
				packet << id << x << y;
				for (player p : clients) {
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
	player you;
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
				// And send it to the server
				if (is_arrow(event.key.code)) {
					packet.clear();
					packet << id << you.position.x << you.position.y;
					you.socket->send(packet);
				}
			break;
			}
		}
		// If you receive data from the server
		if (you.socket->receive(packet) == sf::Socket::Done) {
			int id, x, y;
			packet >> id >> x >> y;
			if (players.size() <= id) players.push_back({ nullptr, {x, y} });
			else players[id].position = sf::Vector2i(x, y);
		}
		window.clear();
		for (player p : players) {
			sf::CircleShape c(10);
			c.setFillColor(sf::Color::Red);
			c.setPosition(sf::Vector2f(p.position));
			window.draw(c);
		}
		window.display();
	}
}