#include "ModuleNetworkingClient.h"


bool  ModuleNetworkingClient::start(const char * serverAddressStr, int serverPort, const char *pplayerName)
{
	playerName = pplayerName;


	// TODO(jesus): TCP connection stuff
	// - Create the socket
	socket = ::socket(AF_INET, SOCK_STREAM, 0);

	//// Set non-blocking socket
	//u_long nonBlocking = 1;
	//int res = ioctlsocket(socket, FIONBIO, &nonBlocking);
	//if (res == SOCKET_ERROR) {
	//	ELOG("Socket error, exiting...");
	//	return false;
	//}

	// - Create the remote address object
	sockaddr_in remoteAddr;
	remoteAddr.sin_family = AF_INET; // IPv4
	remoteAddr.sin_port = htons(serverPort); // Port
	inet_pton(AF_INET, serverAddressStr, &remoteAddr.sin_addr);
	
	// - Connect to the remote address
	int res = connect(socket, (const sockaddr*)&remoteAddr, sizeof(remoteAddr));
	if (res != NO_ERROR)
	{
		ELOG("Error connecting to server");
		return false;
	}
	// - Add the created socket to the managed list of sockets using addSocket()
	addSocket(socket);
	

	// If everything was ok... change the state
	state = ClientState::Start;

	return true;
}

bool ModuleNetworkingClient::isRunning() const
{
	return state != ClientState::Stopped;
}

bool ModuleNetworkingClient::update()
{
	if (state == ClientState::Start)
	{
		OutputMemoryStream packet;
		packet << ClientMessage::Hello;
		packet << playerName;

		if (SendPacket(packet, socket))
			state = ClientState::Logging;
		else
		{
			disconnect();
			state = ClientState::Stopped;
		}
	}

	return true;
}

bool ModuleNetworkingClient::gui()
{
	if (state != ClientState::Stopped)
	{
		// NOTE(jesus): You can put ImGui code here for debugging purposes
		const ImU32 flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		ImGui::Begin("Client Window", nullptr, flags);

		Texture *tex = App->modResources->client;
		ImVec2 texSize(400.0f, 400.0f * tex->height / tex->width);
		ImGui::Image(tex->shaderResource, texSize);

		ImGui::Text("%s connected to the server...", playerName.c_str());

		if (state == ClientState::Connected)
		{
			ImGui::BeginChildFrame(1,ImVec2(400,425));
			ImGui::TextWrapped(textBuff.begin());
			ImGui::EndChildFrame();

			char inputText[128] = "";
			if (ImGui::InputText("##inputText", inputText, 128, ImGuiInputTextFlags_EnterReturnsTrue))
			{
				OutputMemoryStream packet;
				packet << ClientMessage::SendMsg;
				packet << inputText;

				SendPacket(packet, socket);

				std::string msg = inputText;
				msg += "\n";
				textBuff.appendf(inputText);
			}
		}

		ImGui::End();
	}

	return true;
}

void ModuleNetworkingClient::onSocketReceivedData(SOCKET socket, const InputMemoryStream& packet)
{
	ServerMessage serverMessage;
	packet >> serverMessage;

	switch (serverMessage)
	{
	case ServerMessage::Welcome:
		state = ClientState::Connected;
		break;
	case ServerMessage::NoWelcome:
		state = ClientState::Stopped;
		break;
	case ServerMessage::SendMsg:
	{
		std::string msg;
		packet >> msg;
		msg += "\n";
		textBuff.appendf(msg.c_str());
		//LOG("%s", msg.c_str());
	}
		break;
	default:
		break;
	}
}

void ModuleNetworkingClient::onSocketDisconnected(SOCKET socket)
{
	state = ClientState::Stopped;
}

