/**
		Copyright (C) 2012, 2013 Nils Weiss, Patrick Bruenn.

    This file is part of Wifly_Light.

    Wifly_Light is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Wifly_Light is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Wifly_Light.  If not, see <http://www.gnu.org/licenses/>. */

#include "BroadcastReceiver.h"
#include "WiflyControlCli.h"
#include "WiflyControlCmd.h"
#include "WiflyControlException.h"
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <unistd.h>
#include <vector>
#include <sstream>

using std::cin;
using std::cout;

WiflyControlCli::WiflyControlCli(uint32_t addr, uint16_t port)
: mControl(addr, port), mRunning(true)
{
	cout << "Connecting to " << std::hex << addr << ':' << port << std::endl;
}

void WiflyControlCli::Run(void)
{
	const WiflyControlCmd* pCmd;
	ShowHelp();
	string nextCmd;
	while(mRunning)
	{
		cout << "WiflyControlCli: ";
		cin >> nextCmd;

		if("exit" == nextCmd)
		{
			return;
		}
		else if("?" == nextCmd)
		{
			ShowHelp();
		}
		else
		{
			pCmd = WiflyControlCmdBuilder::GetCmd(nextCmd);
			if(NULL != pCmd) {
				pCmd->Run(mControl);
			}
		}
	}
}

void WiflyControlCli::ShowHelp(void) const
{
	cout << "Command reference:" << endl;
	cout << "'?' - this help" << endl;
	cout << "'exit' - terminate cli" << endl;

	size_t i = 0;
	const WiflyControlCmd* pCmd = WiflyControlCmdBuilder::GetCmd(i++);
	while(pCmd != NULL) {
		cout << *pCmd << endl;
		pCmd = WiflyControlCmdBuilder::GetCmd(i++);
	}
	return;
}

int main(int argc, const char* argv[])
{
	BroadcastReceiver receiver(55555);
	std::stringstream logStream;
	std::thread t(std::ref(receiver), std::ref(logStream));

	// wait for user input
	size_t selection;
	std::thread u([&]
	{
		size_t numOfRemotes = 0;
		while(selection >= receiver.NumRemotes())
		{
			sleep(1);
			if(numOfRemotes != receiver.NumRemotes())
			{
				numOfRemotes = receiver.NumRemotes();
				for(int i = 0; i < 100; i++) cout << endl;
				receiver.PrintAllEndpoints(cout);
	}}});
	
	do
	{
		std::cin >> selection;
	} while(selection >= receiver.NumRemotes());
	
	u.join();
	receiver.Stop();
	t.join();
	
	const Endpoint& e = receiver.GetEndpoint(selection);
	WiflyControlCli cli(e.GetIp(), e.GetPort());
	cli.Run();
}
