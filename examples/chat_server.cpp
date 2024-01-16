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
	if (argc == 2) {
		port = atoi(argv[1]);
	} else {
		printf("Usage:\n  %s <PORT>\n", argv[0]);
	}

	printf("Running on port %u\n", port);

	icon7::Initialize();

	rpc.RegisterMessage(
		"SetNickname",
		[](icon7::Peer *peer, const std::string &nickname) -> bool {
			if (peer->userPointer != nullptr) {
				std::string *oldName = (std::string *)(peer->userPointer);
				peers.erase(*oldName);
				delete oldName;
				peer->userPointer = nullptr;
			}
			if (nickname != "") {
				if (peers.find(nickname) != peers.end()) {
					return false;
				}
				peer->userPointer = new std::string(nickname);
				peers[nickname] = peer;
			}
			return true;
		});
	rpc.RegisterMessage("Broadcast",
						[](icon7::Peer *peer, std::string message) {
							std::string nickname = "";
							if (peer->userPointer != nullptr) {
								nickname = *(std::string *)(peer->userPointer);
							}

							peer->host->ForEachPeer([&](icon7::Peer *p2) {
								if (p2 != peer) {
									rpc.Send(p2, icon7::FLAG_RELIABLE,
											 "Broadcasted", nickname, message);
								}
							});
						});
	rpc.RegisterMessage("Msg",
						[](icon7::Peer *peer, const std::string nickname,
						   std::string message) -> bool {
							std::string srcNickname = "";
							if (peer->userPointer != nullptr) {
								srcNickname =
									*(std::string *)(peer->userPointer);
							}
							auto it = peers.find(nickname);
							if (it != peers.end()) {
								rpc.Send(it->second, icon7::FLAG_RELIABLE,
										 "Msg", srcNickname, message);
								return true;
							} else {
								printf("Not found `%s`\n", nickname.c_str());
								return false;
							}
						});

	icon7::HostUStcp *_host = new icon7::HostUStcp();
	_host->Init();
	icon7::Host *host = _host;
	host->ListenOnPort(port);

	host->SetOnDisconnect([](icon7::Peer *peer) {
		if (peer->userPointer != nullptr) {
			std::string *oldName = (std::string *)(peer->userPointer);
			peers.erase(*oldName);
			delete oldName;
			peer->userPointer = nullptr;
		}
	});
	host->SetRpcEnvironment(&rpc);
	host->RunAsync();

	printf("To exit write: quit\n");

	while (true) {
		std::string str;
		std::cin >> str;
		if (str == "quit") {
			host->QueueStopRunning();
			host->WaitStopRunning();
			break;
		}
	}

	delete host;

	icon7::Deinitialize();
	return 0;
}
