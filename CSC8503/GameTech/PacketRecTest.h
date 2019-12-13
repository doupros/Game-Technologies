#pragma once
//#include "NetworkedGame.h"
#include "..\CSC8503Common\NetworkBase.h"
#include "..\CSC8503Common\NetworkObject.h"
#include "..\CSC8503Common\GameClient.h"
#include "..\CSC8503Common\GameServer.h"

class PacketRecTest:public PacketReceiver
{
public: 
	PacketRecTest(string name)
	{
		this->name = name;
	}
	void ReceivePacket(int type, GamePacket* payload, int source) {
		if (type == String_Message) {
			StringPacket* realPacket = (StringPacket*)payload;
			TimePacket* timePacket = (TimePacket*)payload;
			string msg = realPacket->GetStringFromData();
			int time = timePacket->GetTimeFromData();
			std::cout << name << " received message : " << msg << std::endl;
			std::cout << name << " time packet is : " << time << std::endl;
		}
	}
	std::string GetName() { return name; }
protected:
	string name;
	int time;
};

