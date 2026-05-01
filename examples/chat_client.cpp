#include <cstdio>

#include <iostream>

#include <icon7/Time.hpp>
#include <icon7/Peer.hpp>
#include <icon7/Flags.hpp>
#include <icon7/RPCEnvironment.hpp>
#include <icon7/Flags.hpp>
#include <icon7/PeerUStcp.hpp>
#include <icon7/HostUStcp.hpp>
#include "../include/icon7/LoopUS.hpp"
#include "icon7/Forward.hpp"

std::unordered_map<std::string, icon7::Peer *> peers;
icon7::RPCEnvironment rpc;

void ClearLineOutput() {
	printf("\r                                                  \r");
}

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

	rpc.RegisterMessage("Broadcasted", [](icon7::PeerHandle peer,
										  const std::string &nickname,
										  std::string message) {
		ClearLineOutput();
		printf("broadcast from %s: %s\n",
			   nickname == "" ? "<anonymus>" : nickname.c_str(),
			   message.c_str());
	});
	rpc.RegisterMessage("Msg", [](icon7::PeerHandle peer,
								  const std::string &nickname,
								  std::string message) {
		ClearLineOutput();
		printf("private message from %s: %s\n",
			   nickname == "" ? "<anonymus>" : nickname.c_str(),
			   message.c_str());
	});

	std::shared_ptr<icon7::uS::Loop> loop =
		std::make_shared<icon7::uS::Loop>("loop_client");
	loop->Init(1);
	std::shared_ptr<icon7::uS::tcp::Host> _host =
		loop->CreateHost(&rpc, "host_client", false);

	icon7::Host *host = _host.get();
	loop->RunAsync();

	icon7::PeerHandle peer = host->ConnectPromise(argv[1], port).get();

	printf("To exit write: quit\n");

	while (true) {
		ClearLineOutput();
		printf("[%s]:", peer.GetSharedPeer()->IsReadyToUse() ? "ready" : "not_ready");
		std::string str;
		std::getline(std::cin, str);
		if (str.substr(0, 4) == "quit" && str.size() <= 6) {
			icon7::Peer::Disconnect(peer);
			icon7::time::Sleep(icon7::time::milliseconds(50));
			loop->QueueStopRunning();
			loop->WaitStopRunning();
			break;
		} else if (str.substr(0, 5) == "nick ") {
			rpc.Send(peer, icon7::FLAG_RELIABLE, "SetNickname",
					 str.substr(5, -1));
		} else if (str.substr(0, 4) == "msg ") {
			auto end = str.find(' ', 5);
			std::string recipient = str.substr(4, end - 4);
			ClearLineOutput();
			printf("Sending msg to `%s`\n", recipient.c_str());

			rpc.Call(peer, icon7::FLAG_RELIABLE,
					 icon7::OnReturnCallback::Make<bool>(
						 [recipient](icon7::PeerHandle peer, icon7::Flags flags,
									 bool result) -> void {
							 if (result == false) {
								 ClearLineOutput();
								 printf("No user named '%s' found in server\n",
										recipient.c_str());
							 }
						 },
						 [](icon7::PeerHandle peer) -> void {
						 ClearLineOutput();
							 printf(" Sending private message timed out\n");
						 },
						 10000, peer),
					 "Msg", recipient, str.substr(end + 1));
		} else {
			rpc.Send(peer, icon7::FLAG_RELIABLE, "Broadcast", str);
		}
	}

	loop->Destroy();
	loop = nullptr;

	icon7::Deinitialize();
	return 0;
}
