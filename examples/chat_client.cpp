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

	std::shared_ptr<icon7::uS::Loop> loop = std::make_shared<icon7::uS::Loop>();
	loop->Init(1);
	std::shared_ptr<icon7::uS::tcp::Host> _host = loop->CreateHost(false);

	icon7::Host *host = _host.get();
	host->SetRpcEnvironment(&rpc);
	loop->RunAsync();

	std::shared_ptr<icon7::Peer> peer =
		host->ConnectPromise(argv[1], port).get();

	printf("To exit write: quit\n");

	while (true) {
		printf("[%s]:", peer->IsReadyToUse() ? "ready" : "not_ready");
		std::string str;
		std::getline(std::cin, str);
		if (str.substr(0, 4) == "quit" && str.size() <= 6) {
			peer->Disconnect();
			icon7::time::Sleep(icon7::time::milliseconds(50));
			loop->QueueStopRunning();
			loop->WaitStopRunning();
			break;
		} else if (str.substr(0, 5) == "nick ") {
			rpc.Send(peer.get(), icon7::FLAG_RELIABLE, "SetNickname",
					 str.substr(5, -1));
		} else if (str.substr(0, 4) == "msg ") {
			auto end = str.find(' ', 5);
			std::string recipient = str.substr(4, end - 4);
			printf("Sending msg to `%s`\n", recipient.c_str());

			rpc.Call(peer.get(), icon7::FLAG_RELIABLE,
					 icon7::OnReturnCallback::Make<bool>(
						 [recipient](icon7::Peer *peer, icon7::Flags flags,
									 bool result) -> void {
							 if (result == false) {
								 printf("No user named '%s' found in server\n",
										recipient.c_str());
							 }
						 },
						 [](icon7::Peer *peer) -> void {
							 printf(" Sending private message timed out\n");
						 },
						 10000, peer.get()),
					 "Msg", recipient, str.substr(end + 1));
		} else {
			rpc.Send(peer.get(), icon7::FLAG_RELIABLE, "Broadcast", str);
		}
	}

	loop->Destroy();
	loop = nullptr;

	icon7::Deinitialize();
	return 0;
}
