#include <cstdio>

#include <iostream>

#include <icon7/Peer.hpp>
#include <icon7/Flags.hpp>
#include <icon7/RPCEnvironment.hpp>
#include <icon7/Flags.hpp>
#include <icon7/PeerUStcp.hpp>
#include <icon7/HostUStcp.hpp>
#include "../include/icon7/LoopUS.hpp"

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

	std::shared_ptr<icon7::uS::Loop> loop = std::make_shared<icon7::uS::Loop>();
	loop->Init(1);
	std::shared_ptr<icon7::uS::tcp::Host> _host = loop->CreateHost(false);

	icon7::Host *host = _host.get();
	host->SetOnDisconnect([](icon7::Peer *peer) {
		if (peer->userPointer != nullptr) {
			std::string *oldName = (std::string *)(peer->userPointer);
			peers.erase(*oldName);
			delete oldName;
			peer->userPointer = nullptr;
		}
	});
	host->SetRpcEnvironment(&rpc);
	loop->RunAsync();

	auto Listen = [host, port](std::string addr) -> bool {
		auto listeningFuture = host->ListenOnPort(addr, port, icon7::IPv4);
		listeningFuture.wait_for_seconds(2);
		if (listeningFuture.get()) {
			printf("Listening on: %s [%i]\n", addr.c_str(), port);
			return true;
		} else {
			printf("Failed to listen on: %s [%i]\n", addr.c_str(), port);
			return false;
		}
	};
	Listen("");
	Listen("localhost");
	Listen("127.0.0.1");
	Listen("::1");

	printf("To exit write: quit\n");

	while (true) {
		std::string str;
		std::cin >> str;
		if (str == "quit") {
			break;
		}
	}

	loop->Destroy();
	loop = nullptr;

	icon7::Deinitialize();
	return 0;
}
