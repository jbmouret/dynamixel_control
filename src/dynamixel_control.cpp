#include "ros/ros.h"

#include <fstream>

#include "dynamixel.hpp"

#include "dynamixel_control/PositionCtrl.h"
#include "dynamixel_control/SpeedCtrl.h"

#include "dynamixel_control/GetIDs.h"
#include "dynamixel_control/GetActuatorPosition.h"
#include "dynamixel_control/GetActuatorsPositions.h"
#include "dynamixel_control/SetActuatorPosition.h"
#include "dynamixel_control/SetActuatorSpeed.h"
#include "dynamixel_control/SetPower.h"
#include "dynamixel_control/SetActualActuatorsPositions.h"

#define READ_DURATION 0.02

using namespace dynamixel;

Usb2Dynamixel controller;
std::vector<byte_t> ax12_ids;
std::map<byte_t,int> offsets;

void saveOffsets()
{
	std::ofstream file("offsets.conf",std::ofstream::out|std::ofstream::trunc);
	std::map<byte_t,int>::iterator it = offsets.begin();
	do
	{
		file << (int)(*it).first << " " << (int)(*it).second << " ";
		++it;
	} while(it != offsets.end());
	file.close();
}

void loadOffsets()
{
	int key;
	int value;

	std::ifstream file("offsets.conf",std::ifstream::in);
	while (file >> key >> value)
		offsets[(byte_t)key] = value;
	file.close();
}

bool GetIDsService(dynamixel_control::GetIDs::Request  &req,
		dynamixel_control::GetIDs::Response &res)
{
	res.ids = ax12_ids;
}

bool GetActuatorPositionService(dynamixel_control::GetActuatorPosition::Request  &req,
		dynamixel_control::GetActuatorPosition::Response &res)
{
	if(std::find(ax12_ids.begin(),ax12_ids.end(),req.id) != ax12_ids.end())
	{
		dynamixel::Status status;
		try
		{
			controller.send(dynamixel::ax12::GetPosition(req.id));
			controller.recv(READ_DURATION, status);
		}
		catch (Error e)
		{
			ROS_ERROR("%s",e.msg().c_str());
			return -1;
		}

		res.pos = status.decode16();
		return true;
	}
	else
	{
		return false;
	}
}

bool GetActuatorsPositionsService(dynamixel_control::GetActuatorsPositions::Request  &req,
		dynamixel_control::GetActuatorsPositions::Response &res)
{
	dynamixel::Status status;
	std::vector<int> positions;
	try
	{
		for(int i = 0; i < ax12_ids.size(); i++)
		{
			controller.send(dynamixel::ax12::GetPosition(ax12_ids[i]));
			controller.recv(READ_DURATION, status);
			positions.push_back(status.decode16());
		}
	}
	catch (Error e)
	{
		ROS_ERROR("%s",e.msg().c_str());
		return -1;
	}

	res.ids = ax12_ids;
	res.positions = positions;
}

bool SetActuatorSpeedService(dynamixel_control::SetActuatorSpeed::Request  &req,
		dynamixel_control::SetActuatorSpeed::Response &res)
{
	if(std::find(ax12_ids.begin(),ax12_ids.end(),req.id) != ax12_ids.end())
	{
		dynamixel::Status status;
		try
		{
			controller.send(dynamixel::ax12::SetSpeed(req.id, req.speed > 0, abs(req.speed)));
			controller.recv(READ_DURATION, status);
		}
		catch (Error e)
		{
			ROS_ERROR("%s",e.msg().c_str());
			return -1;
		}

		return true;
	}
	else
	{
		return false;
	}
}

void SetActuatorsSpeedsCallback(const dynamixel_control::SpeedCtrl &msg)
{
	dynamixel::Status status;

	std::vector<bool> directions;
	for(int i = 0; i < msg.directions.size(); i++)
		directions.push_back((bool)msg.directions[i]);

	try
	{
		controller.send(dynamixel::ax12::SetSpeeds(msg.ids, directions, msg.speeds));
		controller.recv(READ_DURATION, status);
	}
	catch (Error e)
	{
		ROS_ERROR("%s",e.msg().c_str());
	}
}

bool SetActuatorPositionService(dynamixel_control::SetActuatorPosition::Request  &req,
		dynamixel_control::SetActuatorPosition::Response &res)
{
	if(std::find(ax12_ids.begin(),ax12_ids.end(),req.id) != ax12_ids.end())
	{
		try
		{
			dynamixel::Status status;
			controller.send(dynamixel::ax12::SetPosition(req.id, req.position + offsets[req.id]));
			controller.recv(READ_DURATION, status);
		}
		catch (Error e)
		{
			ROS_ERROR("%s",e.msg().c_str());
			return -1;
		}

		return true;
	}
	else
	{
		ROS_ERROR("Dynamixel n°%d doesnt exist",(int)req.id);
		return false;
	}
}

void SetActuatorsPositionsCallback(const dynamixel_control::PositionCtrl& msg)
{
	dynamixel::Status status;
	try
	{
		std::vector<byte_t> new_ids;
		std::vector<int> new_positions;
		for(int i = 0; i < msg.ids.size(); i++)
		{
			int id = msg.ids[i];
			if(std::find(ax12_ids.begin(),ax12_ids.end(),id) != ax12_ids.end())
			{
				new_ids.push_back(id);
				new_positions.push_back(msg.positions[i] + offsets[id]);
			}
			else
				ROS_ERROR("Dynamixel n°%d doesnt exist",id);
		}
		controller.send(dynamixel::ax12::SetPositions(new_ids, new_positions));
		controller.recv(READ_DURATION, status);
	}
	catch (Error e)
	{
		ROS_ERROR("%s",e.msg().c_str());
	}
}

bool SetPowerService(dynamixel_control::SetPower::Request  &req,
		dynamixel_control::SetPower::Response &res)
{
	dynamixel::Status status;
	try
	{
		for (size_t i = 0; i < ax12_ids.size(); ++i)
		{
			controller.send(dynamixel::ax12::TorqueEnable(ax12_ids[i], req.enable));
			controller.recv(READ_DURATION, status);
		}
	}
	catch (Error e)
	{
		ROS_ERROR("%s",e.msg().c_str());
		return -1;
	}
	return true;
}

bool SetActualActuatorsPositionsService(dynamixel_control::SetActualActuatorsPositions::Request  &req,
		dynamixel_control::SetActualActuatorsPositions::Response &res)
{
	dynamixel::Status status;
	try
	{
		for(int i = 0; i < req.ids.size(); i++)
		{
			int id = req.ids[i];
			if(std::find(ax12_ids.begin(),ax12_ids.end(),id) != ax12_ids.end())
			{
				controller.send(dynamixel::ax12::GetPosition(id));
				controller.recv(READ_DURATION, status);
				offsets[id] = status.decode16() - req.positions[i];
			}
			else
				ROS_ERROR("Dynamixel n°%d doesnt exist",id);
		}

		saveOffsets();
	}
	catch (Error e)
	{
		ROS_ERROR("%s",e.msg().c_str());
		return -1;
	}
	return true;
}

int main(int argc, char** argv)
{
	ros::init(argc, argv, "dynamixel_control");
	ros::NodeHandle nh("/dynamixel_control");

	ros::ServiceServer getIDsService = nh.advertiseService("getids", GetIDsService);
	ros::ServiceServer getPositionService = nh.advertiseService("getposition", GetActuatorPositionService);
	ros::ServiceServer getPositionsService = nh.advertiseService("getpositions", GetActuatorsPositionsService);
	ros::ServiceServer setSpeedService = nh.advertiseService("setspeed", SetActuatorSpeedService);
	ros::ServiceServer setPositionService = nh.advertiseService("setposition", SetActuatorPositionService);
	//ros::ServiceServer setSpeedsService = nh.advertiseService("setspeeds", SetActuatorsSpeedsService);
	ros::ServiceServer setPowerService = nh.advertiseService("setpower", SetPowerService);
	ros::ServiceServer SetActualPositionsService = nh.advertiseService("setactualpositions", SetActualActuatorsPositionsService);

	ros::Subscriber setPositionsSub = nh.subscribe("setpositions", 1, SetActuatorsPositionsCallback);
	ros::Subscriber setSpeedsSub = nh.subscribe("setspeeds", 1, SetActuatorsSpeedsCallback);

	// node params :
	std::string port_name;
	int baudrate;

	nh.param<std::string>("port_name", port_name, "/dev/ttyUSB0");
	nh.param<int>("baudrate", baudrate, B1000000);

	ROS_INFO("Port : %s",port_name.c_str());
	ROS_INFO("Baudrate : %d",baudrate);

	dynamixel::Status status;
	try
	{
		controller.open_serial(port_name, baudrate);
		ROS_INFO("Serial port open");

		controller.scan_ax12s();
		ax12_ids = controller.ax12_ids();
		for(int i = 0; i < ax12_ids.size(); i++)
			offsets.insert(std::pair<byte_t,int>(ax12_ids[i],0));
		ROS_INFO("%d dynamixels are connected",(int)ax12_ids.size());;
		for (size_t i = 0; i < ax12_ids.size(); ++i)
		{
			ROS_INFO("id : %d",(int)ax12_ids[i]);
			controller.send(dynamixel::ax12::SetMaxTorque(ax12_ids[i],1023));
			controller.recv(READ_DURATION, status);
		}

	}
	catch (Error e)
	{
		ROS_ERROR("%s",e.msg().c_str());
		return -1;
	}

	loadOffsets();

	ros::spin();

	// Stopping all wheels
	std::vector<bool> directions;
	std::vector<int> speeds;
	for(int i = 0; i < ax12_ids.size(); i++)
	{
		directions.push_back(false);
		speeds.push_back(0);
	}
	try
	{
		controller.send(dynamixel::ax12::SetSpeeds(ax12_ids, directions, speeds));
		controller.recv(READ_DURATION, status);
	}
	catch (Error e)
	{
		ROS_ERROR("%s",e.msg().c_str());
	}


	return 0;

}
