#pragma once

struct entity {
	sf::Vector2f position { 0, 0 };
	double direction;
};

sf::Packet& operator<<(sf::Packet& packet, const entity& entity) {
	return packet << entity.position.x << entity.position.y << entity.direction;
}

sf::Packet& operator>>(sf::Packet& packet, entity& entity) {
	return packet >> entity.position.x >> entity.position.y >> entity.direction;
}

struct player : entity { };

struct client : player {
	sf::TcpSocket* socket = new sf::TcpSocket;
};

inline double d2r(double d) {
	return d * M_PI / 180;
}

struct bullet : entity {
	bullet(sf::Vector2f pos, double dir) : entity {pos, dir} { }
	void tick() {
		position.x += cos(d2r(direction));
		position.y += sin(d2r(direction));
	}
};