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
			packet << static_cast<int>(clients.size());
			client.socket->send(packet);
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
				for (player p : clients) {
					packet << p.position.x << p.position.y;
				}
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
	you.socket->connect("localhost", PORT);
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
		sf::Event event;
		while (window.pollEvent(event)) {
			switch (event.type) {
			case sf::Event::Closed: window.close(); break;
			case sf::Event::KeyPressed:
				if (event.key.code == sf::Keyboard::Left) {
					you.position.x--;
					packet.clear();
					packet << id << you.position.x << you.position.y;
					you.socket->send(packet);
				}
			break;
			}
		}
		if (you.socket->receive(packet) == sf::Socket::Done) {
			int x, y;
			while (packet >> x >> y) {
				players.push_back({ nullptr, {x, y} });
			}
		}
		window.clear();
		for (player p : players) {
			sf::CircleShape c(30);
			c.setFillColor(sf::Color::Red);
			c.setPosition(sf::Vector2f(p.position));
			window.draw(c);
		}
		window.display();
	}
}