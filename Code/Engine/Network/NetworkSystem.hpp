#pragma once
#include <string>
#include <vector>
#include <deque>
#include <cstdint>

//-----------------------------------------------------------------------------------------------
/*
This Network System is very simple, it can only poll string(check \0) from the buffer now.
we do not support any protocol
and it will store strings to send, and at begin frame or endframe it send these out.
only TCP + IPV4
*/


typedef intptr_t SocketHandle;
constexpr SocketHandle INVALID_SOCKET_HANDLE = static_cast<SocketHandle>(-1);
std::string WsaErrorCodeToString(int errorCode);
//-----------------------------------------------------------------------------------------------
struct NetworkConfig
{

};

//-----------------------------------------------------------------------------------------------
enum class NetState
{
	INACTIVE = 0,       // Before NS::Startup()
	IDLE,               // Has called WSAStartup, but not listening or connecting
	SERVER_LISTENING,    // 
	CLIENT_CONNECTING,   // 
	CLIENT_CONNECTED,    // 
	COUNT
};

struct NetClientInfo
{
	SocketHandle			m_socket = INVALID_SOCKET_HANDLE;
	std::vector<uint8_t>	m_recvBuffer;
	std::vector<uint8_t>	m_sendBuffer;
	std::deque<std::string> m_outgoingStrings;
};

//-----------------------------------------------------------------------------------------------
class NetworkSystem
{
public:
	NetworkSystem(NetworkConfig const& config);
	~NetworkSystem() = default;
	NetworkSystem(const NetworkSystem&) = delete;
	NetworkSystem& operator=(const NetworkSystem&) = delete;

	void Startup();
	void Shutdown();

	void BeginFrame();
	void EndFrame();

	bool StartServer(unsigned short listenPort);
	void StopServer();

	bool StartClient(std::string const& serverIP, unsigned short serverPort);
	void StopClient();

public:
	NetState GetState() const;

	// Server Broadcast or Client send to server
	void QueueOutgoingString(const std::string& s);
	// Single point to client (not used now)
	void QueueOutgoingStringToClient(SocketHandle client, const std::string& s);

	std::vector<std::string> RetrieveIncomingStrings();

	bool m_pendingDisconnected = false; // will try to send the last message and stop
private:
	NetworkConfig m_config;
	NetState m_state = NetState::INACTIVE;

private:
	//-----------------------------------------------------------------------------------------------
	// Server
	SocketHandle m_listenSocket = INVALID_SOCKET_HANDLE;
	std::vector<NetClientInfo*> m_serverClients;

private:
	//-----------------------------------------------------------------------------------------------
	// Client
	SocketHandle m_connectionToServerSocket = INVALID_SOCKET_HANDLE;
	std::vector<uint8_t> m_clientRecvBuffer;
	std::vector<uint8_t> m_clientSendBuffer;

private:
	// Game Code will interact with these strings
	std::deque<std::string> m_outgoingStrings; // wait for sending (broadcasting for server)
	std::deque<std::string> m_incomingStrings; // received strings, wait for taking out by game code

	//void BufferOutgoingStrings(std::vector<uint8_t>& sendBuffer);

	void ExtractIncomingStrings(std::vector<uint8_t>& recvBuffer);
	void BufferOutgoingStrings(std::deque<std::string>& stringQueue, std::vector<uint8_t>& sendBuffer);
	void BroadcastOutgoingStrings(const std::deque<std::string>& stringQueue);

	static constexpr size_t RECV_BUFFER_SIZE = 2048;
	static constexpr int MAX_RECV_PER_CLIENT_PER_FRAME = 10;

private:
	void AcceptNetClients();
	void ServerSendRecv();

	void ClientConnecting();
	void ClientSendRecv();

	void CloseSocket(SocketHandle& sock);
};

