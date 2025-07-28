#include "Engine/Network/NetworkSystem.hpp"
#include "Engine/Core/EngineCommon.hpp"
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <WinSock2.h>
#include <WS2TCPIP.h>
#pragma comment(lib, "Ws2_32.lib")


//-----------------------------------------------------------------------------------------------
NetworkSystem::NetworkSystem(NetworkConfig const& config)
	: m_config(config)
{
	
}

void NetworkSystem::Startup()
{
	WSADATA wsaData;
	// directly return error code, not need to get last error
	int errorCode = WSAStartup(MAKEWORD(2, 2), &wsaData); // Windows Sockets API Version 2.2 (0x00000202)
	if (errorCode != 0)
	{
		ERROR_AND_DIE(Stringf("WSAStartup failed: %s", WsaErrorCodeToString(errorCode).c_str()));
	}

	m_state = NetState::IDLE;
}


void NetworkSystem::Shutdown()
{
	StopServer();
	StopClient();
	if (m_state != NetState::INACTIVE)
	{
		int ret = WSACleanup();
		if (ret == SOCKET_ERROR)
		{
			int errorCode = WSAGetLastError();
			ERROR_RECOVERABLE(Stringf("WSACleanup failed: %s", WsaErrorCodeToString(errorCode).c_str()));
		}
		m_state = NetState::INACTIVE;
	}
}

void NetworkSystem::BeginFrame()
{
	switch (m_state)
	{
	case NetState::SERVER_LISTENING:
		AcceptNetClients();
		ServerSendRecv();
		break;
	case NetState::CLIENT_CONNECTING:
		ClientConnecting();
		break;
	case NetState::CLIENT_CONNECTED:
		ClientSendRecv();
		break;
	default:
		break;
	}
}

void NetworkSystem::EndFrame()
{
	if (m_pendingDisconnected)
	{

		if (m_state == NetState::SERVER_LISTENING)
		{
			ServerSendRecv();
			StopServer();
		}
		else if (m_state == NetState::CLIENT_CONNECTED)
		{
			ClientSendRecv();
			StopClient();
		}

		m_pendingDisconnected = false;
	}
}

bool NetworkSystem::StartServer(unsigned short listenPort)
{
	if (m_state != NetState::IDLE)
	{
		DebuggerPrintf("Server: Could not start server, network system is not idle.\n");
		return false;
	}

	SOCKET listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listenSock == INVALID_SOCKET)
	{
		int errorCode = WSAGetLastError();
		DebuggerPrintf("Server: Could not create a listen socket: %s\n", WsaErrorCodeToString(errorCode).c_str());
		return false;
	}

	unsigned long mode = 1; // non-blocking
	int ret = ioctlsocket(listenSock, FIONBIO, &mode);
	if (ret == SOCKET_ERROR)
	{
		int errorCode = WSAGetLastError();
		DebuggerPrintf("Server: Could not set the server listen socket to non-blocking: %s\n", WsaErrorCodeToString(errorCode).c_str());
		closesocket(listenSock);
		return false;
	}

	sockaddr_in addr = {};
	addr.sin_family = AF_INET;
	addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(listenPort);

	ret = bind(listenSock, (sockaddr*)&addr, (int)sizeof(addr));
	if (ret == SOCKET_ERROR)
	{
		int errorCode = WSAGetLastError();
		DebuggerPrintf("Server: Could not bind the server listen socket to port: %s\n", WsaErrorCodeToString(errorCode).c_str());
		closesocket(listenSock);
		return false;
	}

	ret = listen(listenSock, SOMAXCONN);
	if (ret == SOCKET_ERROR)
	{
		int errorCode = WSAGetLastError();
		DebuggerPrintf("Server: Could not bind the server listen socket to port: %s\n", WsaErrorCodeToString(errorCode).c_str());
		closesocket(listenSock);
		return false;
	}

	m_listenSocket = (SocketHandle)listenSock;
	m_state = NetState::SERVER_LISTENING;
	m_outgoingStrings.clear();
	m_incomingStrings.clear();
	DebuggerPrintf("Server: start listening on port %d\n", listenPort);
	return true;
}

void NetworkSystem::StopServer()
{
	CloseSocket(m_listenSocket);

	for (NetClientInfo* client : m_serverClients)
	{
		if (client)
		{
			CloseSocket(client->m_socket);
		}
	}

	m_serverClients.clear();
	if (m_state == NetState::SERVER_LISTENING)
	{
		m_state = NetState::IDLE;
	}
}

bool NetworkSystem::StartClient(std::string const& serverIP, unsigned short serverPort)
{
	if (m_state != NetState::IDLE)
	{
		DebuggerPrintf("Client: Could not start client, network system is not idle.\n");
		return false;
	}

	SOCKET connSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (connSock == INVALID_SOCKET) {
		int errorCode = WSAGetLastError();
		DebuggerPrintf("Client: Could not create a client connection socket: %s\n", WsaErrorCodeToString(errorCode).c_str());
		return false;
	}
	
	unsigned long mode = 1; // non-blocking
	int ret = ioctlsocket(connSock, FIONBIO, &mode);
	if (ret == SOCKET_ERROR)
	{
		int errorCode = WSAGetLastError();
		DebuggerPrintf("Client: Could not set the client connection socket to non-blocking: %s\n", WsaErrorCodeToString(errorCode).c_str());
		closesocket(connSock);
		return false;
	}

	sockaddr_in addr = {};
	addr.sin_family = AF_INET; // ipv4
	ret = inet_pton(AF_INET, serverIP.c_str(), &addr.sin_addr);
	if (ret != 1) 
	{
		DebuggerPrintf("Client: inet_pton() failed for IP: %s\n", serverIP.c_str());
		closesocket(connSock);
		return false;
	}
	addr.sin_port = htons(serverPort);

	ret = connect(connSock, (sockaddr*)(&addr), (int)sizeof(addr));
	if (ret == SOCKET_ERROR) 
	{
		int errorCode = WSAGetLastError();
		if (errorCode != WSAEWOULDBLOCK && errorCode != WSAEALREADY) // multiple times? WSAEALREADY
		{
			DebuggerPrintf("Client: connect() failed: %s\n", WsaErrorCodeToString(errorCode).c_str());
			closesocket(connSock);
			return false;
		}
	}

	m_connectionToServerSocket = (SocketHandle)connSock;
	m_clientRecvBuffer.clear();
	m_clientSendBuffer.clear();
	m_outgoingStrings.clear();
	m_incomingStrings.clear();
	m_state = NetState::CLIENT_CONNECTING;
	DebuggerPrintf("Client: connecting to %s:%d\n", serverIP.c_str(), serverPort);
	return true;
}

void NetworkSystem::StopClient()
{
	CloseSocket(m_connectionToServerSocket);

	m_clientRecvBuffer.clear();
	m_clientSendBuffer.clear();
	if (m_state == NetState::CLIENT_CONNECTING || m_state == NetState::CLIENT_CONNECTED)
	{
		m_state = NetState::IDLE;
	}
}

NetState NetworkSystem::GetState() const
{
	return m_state;
}

void NetworkSystem::QueueOutgoingString(const std::string& s)
{
	//if (!(m_state == NetState::SERVER_LISTENING || m_state == NetState::CLIENT_CONNECTED)) 
	//{
	//	DebuggerPrintf("Network System is not listening or connected.\n");
	//	return;
	//}
	m_outgoingStrings.push_back(s);
}

void NetworkSystem::QueueOutgoingStringToClient(SocketHandle client, const std::string& s)
{
	if (m_state != NetState::SERVER_LISTENING) return;
	for (NetClientInfo* c : m_serverClients) 
	{
		if (c != nullptr && c->m_socket == client) 
		{
			c->m_outgoingStrings.push_back(s);
			break;
		}
	}
}

std::vector<std::string> NetworkSystem::RetrieveIncomingStrings()
{
	std::vector<std::string> result;
	while (!m_incomingStrings.empty()) 
	{
		result.push_back(m_incomingStrings.front());
		m_incomingStrings.pop_front();
	}
	return result;
}

void NetworkSystem::ExtractIncomingStrings(std::vector<uint8_t>& recvBuffer)
{
	size_t start = 0;
	while (true) 
	{
		auto it = std::find(recvBuffer.begin() + start, recvBuffer.end(), '\0');
		if (it == recvBuffer.end()) break;

		size_t end = it - recvBuffer.begin();
		m_incomingStrings.emplace_back(reinterpret_cast<const char*>(&recvBuffer[start]), end - start);
		start = end + 1;
	}
	if (start > 0) 
	{
		recvBuffer.erase(recvBuffer.begin(), recvBuffer.begin() + start);
	}
}

void NetworkSystem::BufferOutgoingStrings(std::deque<std::string>& stringQueue, std::vector<uint8_t>& sendBuffer)
{
	while (!stringQueue.empty()) 
	{
		std::string const& s = stringQueue.front();
		sendBuffer.insert(sendBuffer.end(), s.begin(), s.end());
		sendBuffer.push_back('\0');
		stringQueue.pop_front();
	}
}

void NetworkSystem::BroadcastOutgoingStrings(const std::deque<std::string>& stringQueue)
{
	for (NetClientInfo* client : m_serverClients) 
	{
		for (const std::string& s : stringQueue) 
		{
			client->m_sendBuffer.insert(client->m_sendBuffer.end(), s.begin(), s.end());
			client->m_sendBuffer.push_back('\0');
		}
	}
}

void NetworkSystem::AcceptNetClients()
{
	while (true)
	{
		SOCKET newClientSock = accept((SOCKET)m_listenSocket, NULL, NULL);
		if (newClientSock == INVALID_SOCKET) {
			int errorCode = WSAGetLastError();
			if (errorCode == WSAEWOULDBLOCK) break;
			DebuggerPrintf("Server: accept() error\n");
			break;
		}

		unsigned long mode = 1; // non-blocking
		int ret = ioctlsocket(newClientSock, FIONBIO, &mode);
		if (ret == SOCKET_ERROR)
		{
			int errorCode = WSAGetLastError();
			DebuggerPrintf("Server: Could not set the new client socket to non-blocking: %s\n", WsaErrorCodeToString(errorCode).c_str());
			closesocket(newClientSock);
			continue;
		}

		NetClientInfo* client = new NetClientInfo();
		client->m_socket = newClientSock;
		client->m_recvBuffer.reserve(RECV_BUFFER_SIZE);
		client->m_sendBuffer.reserve(RECV_BUFFER_SIZE);
		m_serverClients.push_back(client);
		DebuggerPrintf("Server: new client accepted\n");
	}
}

void NetworkSystem::ServerSendRecv()
{
	//-----------------------------------------------------------------------------------------------
	// Push strings to buffer
	BroadcastOutgoingStrings(m_outgoingStrings);
	m_outgoingStrings.clear();
	
	for (NetClientInfo* client : m_serverClients) 
	{
		BufferOutgoingStrings(client->m_outgoingStrings, client->m_sendBuffer); // single point
	}


	//-----------------------------------------------------------------------------------------------
	// Send and Recv
	for (NetClientInfo* client : m_serverClients) 
	{
		// Send
		while (!client->m_sendBuffer.empty()) 
		{
			int sent = send((SOCKET)client->m_socket,
				reinterpret_cast<const char*>(client->m_sendBuffer.data()),
				(int)client->m_sendBuffer.size(), 0);
			if (sent > 0) 
			{
				client->m_sendBuffer.erase(client->m_sendBuffer.begin(), client->m_sendBuffer.begin() + sent);
			}
			else {
				int errorCode = WSAGetLastError();
				if (errorCode == WSAEWOULDBLOCK) break;
				DebuggerPrintf("Server: send() error, closing client: %s\n", WsaErrorCodeToString(errorCode).c_str());
				CloseSocket(client->m_socket);
				break;
			}
			// sent will not be zero?
		}
		// Recv
		uint8_t temp[RECV_BUFFER_SIZE];
		int recvCount = 0;
		while (++recvCount <= MAX_RECV_PER_CLIENT_PER_FRAME)
		{
			int recvd = recv((SOCKET)client->m_socket, (char*)temp, RECV_BUFFER_SIZE, 0);
			if (recvd > 0) 
			{
				// Read data from the socket into a buffer
				client->m_recvBuffer.insert(client->m_recvBuffer.end(), temp, temp + recvd);

				// Process Data
				ExtractIncomingStrings(client->m_recvBuffer);
			}
			else if (recvd == 0) // the connection has been gracefully closed (by peer?)
			{
				DebuggerPrintf("Server: client disconnected (recv returns 0)\n");
				CloseSocket(client->m_socket);
				break;
			}
			else 
			{
				int errorCode = WSAGetLastError();
				if (errorCode == WSAEWOULDBLOCK) break; // nothing new to recv
				DebuggerPrintf("Server: recv() error, closing client: %s\n", WsaErrorCodeToString(errorCode).c_str());
				CloseSocket(client->m_socket);
				break;
			}
		}
	}
	// clean disconnected client (-1)
	m_serverClients.erase(
		std::remove_if(m_serverClients.begin(), m_serverClients.end(),
			[](NetClientInfo const* c) { return c->m_socket == -1; }),
		m_serverClients.end()
	);
}

void NetworkSystem::ClientConnecting()
{
	// FD stands for “File Descriptor” but really means “socket” in this context, and “set” here is just a C-style container like std::vector
	fd_set writeSockets;	// a list of sockets that can be written-to	
	fd_set exceptSockets;	// a list of sockets with errors
	FD_ZERO(&writeSockets);	// like std::vector.clear()
	FD_ZERO(&exceptSockets);	// like std::vector.clear()
	FD_SET((SOCKET)m_connectionToServerSocket, &writeSockets);	// like .push_back()		// CLIENT-SPECIFIC
	FD_SET((SOCKET)m_connectionToServerSocket, &exceptSockets);	// like .push_back()	
	timeval waitTime = { };

	int ret = select(0, NULL, &writeSockets, &exceptSockets, &waitTime);
	// zero if the time limit expired
	// After select only available fd(socket) is kept in the set
	if (ret == SOCKET_ERROR)
	{
		int errorCode = WSAGetLastError();
		DebuggerPrintf("Client: select() error, please StartClient again: %s\n", WsaErrorCodeToString(errorCode).c_str());
		StopClient();
		return;
	}

	if (FD_ISSET((SOCKET)m_connectionToServerSocket, &exceptSockets)) 
	{
		int error = 0;
		int len = sizeof(error);
		getsockopt((SOCKET)m_connectionToServerSocket, SOL_SOCKET, SO_ERROR, (char*)&error, &len);
		DebuggerPrintf("Client: connect failed, please start client again, error code: %d (%s)\n", error, WsaErrorCodeToString(error).c_str());

		StopClient();
		return;
	}
	if (FD_ISSET((SOCKET)m_connectionToServerSocket, &writeSockets)) 
	{
		m_state = NetState::CLIENT_CONNECTED;
		DebuggerPrintf("Client: connected successfully.\n");
	}
}

void NetworkSystem::ClientSendRecv()
{
	BufferOutgoingStrings(m_outgoingStrings, m_clientSendBuffer);

	while (!m_clientSendBuffer.empty()) 
	{
		int sent = send((SOCKET)m_connectionToServerSocket,
			reinterpret_cast<const char*>(m_clientSendBuffer.data()),
			(int)m_clientSendBuffer.size(), 0);
		if (sent > 0) 
		{
			m_clientSendBuffer.erase(m_clientSendBuffer.begin(), m_clientSendBuffer.begin() + sent);
		}
		else 
		{
			int errorCode = WSAGetLastError();
			if (errorCode == WSAEWOULDBLOCK) break;
			DebuggerPrintf("Client: send() error, closing client: %s\n", WsaErrorCodeToString(errorCode).c_str());
			StopClient();
			return;
		}
		// sent will not be zero?
	}

	uint8_t temp[RECV_BUFFER_SIZE];
	int recvCount = 0;
	while (++recvCount <= MAX_RECV_PER_CLIENT_PER_FRAME)
	{
		int recvd = recv((SOCKET)m_connectionToServerSocket, (char*)temp, RECV_BUFFER_SIZE, 0);
		if (recvd > 0) 
		{
			m_clientRecvBuffer.insert(m_clientRecvBuffer.end(), temp, temp + recvd);

			// Process data
			ExtractIncomingStrings(m_clientRecvBuffer);
		}
		else if (recvd == 0) 
		{
			DebuggerPrintf("Client: server closed the connection\n");
			StopClient();
			return;
		}
		else {
			int errorCode = WSAGetLastError();
			if (errorCode == WSAEWOULDBLOCK) break; // nothing new to recv
			DebuggerPrintf("Client: recv() error, closing client: %s\n", WsaErrorCodeToString(errorCode).c_str());
			StopClient();
			return;
		}
	}
}

void NetworkSystem::CloseSocket(SocketHandle& sock)
{
	if (sock != -1) // Invalid Socket
	{
		closesocket((SOCKET)sock);
		sock = -1;
	}
}

std::string WsaErrorCodeToString(int errorCode)
{
	switch (errorCode)
	{
	case 6:     return "WSA_INVALID_HANDLE: Specified event object handle is invalid.";
	case 8:     return "WSA_NOT_ENOUGH_MEMORY: Insufficient memory available.";
	case 87:    return "WSA_INVALID_PARAMETER: One or more parameters are invalid.";
	case 995:   return "WSA_OPERATION_ABORTED: Overlapped operation aborted.";
	case 996:   return "WSA_IO_INCOMPLETE: Overlapped I/O event object not in signaled state.";
	case 997:   return "WSA_IO_PENDING: Overlapped operations will complete later.";
	case 10004: return "WSAEINTR: Interrupted function call.";
	case 10009: return "WSAEBADF: File handle is not valid.";
	case 10013: return "WSAEACCES: Permission denied.";
	case 10014: return "WSAEFAULT: Bad address.";
	case 10022: return "WSAEINVAL: Invalid argument.";
	case 10024: return "WSAEMFILE: Too many open files.";
	case 10035: return "WSAEWOULDBLOCK: Resource temporarily unavailable.";
	case 10036: return "WSAEINPROGRESS: Operation now in progress.";
	case 10037: return "WSAEALREADY: Operation already in progress.";
	case 10038: return "WSAENOTSOCK: Socket operation on nonsocket.";
	case 10039: return "WSAEDESTADDRREQ: Destination address required.";
	case 10040: return "WSAEMSGSIZE: Message too long.";
	case 10041: return "WSAEPROTOTYPE: Protocol wrong type for socket.";
	case 10042: return "WSAENOPROTOOPT: Bad protocol option.";
	case 10043: return "WSAEPROTONOSUPPORT: Protocol not supported.";
	case 10044: return "WSAESOCKTNOSUPPORT: Socket type not supported.";
	case 10045: return "WSAEOPNOTSUPP: Operation not supported.";
	case 10046: return "WSAEPFNOSUPPORT: Protocol family not supported.";
	case 10047: return "WSAEAFNOSUPPORT: Address family not supported by protocol family.";
	case 10048: return "WSAEADDRINUSE: Address already in use.";
	case 10049: return "WSAEADDRNOTAVAIL: Cannot assign requested address.";
	case 10050: return "WSAENETDOWN: Network is down.";
	case 10051: return "WSAENETUNREACH: Network is unreachable.";
	case 10052: return "WSAENETRESET: Network dropped connection on reset.";
	case 10053: return "WSAECONNABORTED: Software caused connection abort.";
	case 10054: return "WSAECONNRESET: Connection reset by peer.";
	case 10055: return "WSAENOBUFS: No buffer space available.";
	case 10056: return "WSAEISCONN: Socket is already connected.";
	case 10057: return "WSAENOTCONN: Socket is not connected.";
	case 10058: return "WSAESHUTDOWN: Cannot send after socket shutdown.";
	case 10059: return "WSAETOOMANYREFS: Too many references.";
	case 10060: return "WSAETIMEDOUT: Connection timed out.";
	case 10061: return "WSAECONNREFUSED: Connection refused.";
	case 10062: return "WSAELOOP: Cannot translate name.";
	case 10063: return "WSAENAMETOOLONG: Name too long.";
	case 10064: return "WSAEHOSTDOWN: Host is down.";
	case 10065: return "WSAEHOSTUNREACH: No route to host.";
	case 10066: return "WSAENOTEMPTY: Directory not empty.";
	case 10067: return "WSAEPROCLIM: Too many processes.";
	case 10068: return "WSAEUSERS: User quota exceeded.";
	case 10069: return "WSAEDQUOT: Disk quota exceeded.";
	case 10070: return "WSAESTALE: Stale file handle reference.";
	case 10071: return "WSAEREMOTE: Item is remote.";
	case 10091: return "WSASYSNOTREADY: Network subsystem is unavailable.";
	case 10092: return "WSAVERNOTSUPPORTED: Winsock.dll version out of range.";
	case 10093: return "WSANOTINITIALISED: Successful WSAStartup not yet performed.";
	case 10101: return "WSAEDISCON: Graceful shutdown in progress.";
	case 10102: return "WSAENOMORE: No more results.";
	case 10103: return "WSAECANCELLED: Call has been canceled.";
	case 10104: return "WSAEINVALIDPROCTABLE: Procedure call table is invalid.";
	case 10105: return "WSAEINVALIDPROVIDER: Service provider is invalid.";
	case 10106: return "WSAEPROVIDERFAILEDINIT: Service provider failed to initialize.";
	case 10107: return "WSASYSCALLFAILURE: System call failure.";
	case 10108: return "WSASERVICE_NOT_FOUND: Service not found.";
	case 10109: return "WSATYPE_NOT_FOUND: Class type not found.";
	case 10110: return "WSA_E_NO_MORE: No more results.";
	case 10111: return "WSA_E_CANCELLED: Call was canceled.";
	case 10112: return "WSAEREFUSED: Database query was refused.";
	case 11001: return "WSAHOST_NOT_FOUND: Host not found.";
	case 11002: return "WSATRY_AGAIN: Nonauthoritative host not found.";
	case 11003: return "WSANO_RECOVERY: This is a nonrecoverable error.";
	case 11004: return "WSANO_DATA: Valid name, no data record of requested type.";
	case 11005: return "WSA_QOS_RECEIVERS: QoS receivers.";
	case 11006: return "WSA_QOS_SENDERS: QoS senders.";
	case 11007: return "WSA_QOS_NO_SENDERS: No QoS senders.";
	case 11008: return "WSA_QOS_NO_RECEIVERS: QoS no receivers.";
	case 11009: return "WSA_QOS_REQUEST_CONFIRMED: QoS request confirmed.";
	case 11010: return "WSA_QOS_ADMISSION_FAILURE: QoS admission error.";
	case 11011: return "WSA_QOS_POLICY_FAILURE: QoS policy failure.";
	case 11012: return "WSA_QOS_BAD_STYLE: QoS bad style.";
	case 11013: return "WSA_QOS_BAD_OBJECT: QoS bad object.";
	case 11014: return "WSA_QOS_TRAFFIC_CTRL_ERROR: QoS traffic control error.";
	case 11015: return "WSA_QOS_GENERIC_ERROR: QoS generic error.";
	case 11016: return "WSA_QOS_ESERVICETYPE: QoS service type error.";
	case 11017: return "WSA_QOS_EFLOWSPEC: QoS flowspec error.";
	case 11018: return "WSA_QOS_EPROVSPECBUF: Invalid QoS provider buffer.";
	case 11019: return "WSA_QOS_EFILTERSTYLE: Invalid QoS filter style.";
	case 11020: return "WSA_QOS_EFILTERTYPE: Invalid QoS filter type.";
	case 11021: return "WSA_QOS_EFILTERCOUNT: Incorrect QoS filter count.";
	case 11022: return "WSA_QOS_EOBJLENGTH: Invalid QoS object length.";
	case 11023: return "WSA_QOS_EFLOWCOUNT: Incorrect QoS flow count.";
	case 11024: return "WSA_QOS_EUNKOWNPSOBJ: Unrecognized QoS object.";
	case 11025: return "WSA_QOS_EPOLICYOBJ: Invalid QoS policy object.";
	case 11026: return "WSA_QOS_EFLOWDESC: Invalid QoS flow descriptor.";
	case 11027: return "WSA_QOS_EPSFLOWSPEC: Invalid QoS provider-specific flowspec.";
	case 11028: return "WSA_QOS_EPSFILTERSPEC: Invalid QoS provider-specific filterspec.";
	case 11029: return "WSA_QOS_ESDMODEOBJ: Invalid QoS shape discard mode object.";
	case 11030: return "WSA_QOS_ESHAPERATEOBJ: Invalid QoS shaping rate object.";
	case 11031: return "WSA_QOS_RESERVED_PETYPE: Reserved policy QoS element type.";
	default:
		return "Unknown Winsock error code: " + std::to_string(errorCode);
	}
}
