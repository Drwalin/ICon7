#include <cstdio>

#include <iostream>
#include <chrono>
#include <thread>

#include <icon7/Host.hpp>
#include <icon7/Peer.hpp>

int main(int argc, char **argv)
{
	uint16_t port = 12345;
	if (argc == 2) {
		port = atoi(argv[1]);
	} else {
		printf("Usage:\n  %s <PORT>\n", argv[0]);
	}

	printf("Running on port %u\n", port);

	icon7::Initialize();

	icon7::Host *host = icon7::Host::MakeGameNetworkingSocketsHost(port);

	host->SetReceive([](icon7::Peer *peer, icon7::ByteReader &reader,
					   icon7::Flags flags) {
		peer->_InternalSend(reader.data(), reader.get_remaining_bytes(), flags);
		printf("Received: `");
		fwrite(reader.data(), reader.get_remaining_bytes(), 1, stdout);
		printf("`\n");
	});
	host->RunAsync();

	printf("To exit write: quit\n");

	while (true) {
		std::string str;
		std::cin >> str;
		if (str == "quit") {
			host->Stop();
			host->WaitStop();
			break;
		}
	}

	delete host;

	icon7::Deinitialize();
	return 0;
}
