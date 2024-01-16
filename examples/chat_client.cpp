#include <cstdio>

#include <iostream>
#include <chrono>
#include <thread>

#include <icon7/Peer.hpp>
#include <icon7/Flags.hpp>
#include <icon7/RPCEnvironment.hpp>
#include <icon7/Flags.hpp>
#include <icon7/PeerUStcp.hpp>
#include <icon7/HostUStcp.hpp>

std::unordered_map<std::string, icon7::Peer *> peers;
icon7::RPCEnvironment rpc;

int main(int argc, char **argv)
{
	uint16_t port = 12345;
	if (argc != 3) {
		printf("Usage:\n  %s <IP_ADDRES> <PORT>\n", argv[0]);
	} else {
		port = atoi(argv[2]);
	}

	printf("Running on port %u\n", port);

	icon7::Initialize();

	rpc.RegisterMessage("Broadcasted", [](icon7::Peer *peer,
										  const std::string &nickname,
										  std::string message) {
		printf("broadcast from %s: %s\n",
			   nickname == "" ? "<anonymus>" : nickname.c_str(),
			   message.c_str());
	});
	rpc.RegisterMessage("Msg", [](icon7::Peer *peer,
								  const std::string &nickname,
								  std::string message) {
		printf("private message from %s: %s\n",
			   nickname == "" ? "<anonymus>" : nickname.c_str(),
			   message.c_str());
	});

	icon7::HostUStcp *_host = new icon7::HostUStcp();
	_host->Init();
	icon7::Host *host = _host;
	host->SetRpcEnvironment(&rpc);
	host->RunAsync();

	icon7::Peer *peer = host->ConnectPromise(argv[1], port).get();

	printf("To exit write: quit\n");

	while (true) {
		std::string str;
		std::getline(std::cin, str);
		if (str.substr(0, 4) == "quit" && str.size() <= 6) {
			peer->Disconnect();
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
			host->QueueStopRunning();
			host->WaitStopRunning();
			break;
		} else if (str.substr(0, 5) == "nick ") {
			rpc.Send(peer, icon7::FLAG_RELIABLE, "SetNickname",
					 str.substr(5, -1));
		} else if (str.substr(0, 4) == "msg ") {
			auto end = str.find(' ', 5);
			std::string recipient = str.substr(4, end - 4);
			printf("Sending msg to `%s`\n", recipient.c_str());
			rpc.Send(peer, icon7::FLAG_RELIABLE, "Msg", recipient,
					 str.substr(end + 1));
		} else {
			rpc.Send(peer, icon7::FLAG_RELIABLE, "Broadcast", str);
		}
	}

	delete host;

	icon7::Deinitialize();
	return 0;
}
