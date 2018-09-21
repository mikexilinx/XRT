#include "continuous_profile.h"

#include <stdlib.h>
#include <thread>
#include <chrono>

namespace XCL {

ThreadMonitor::~ThreadMonitor() {
	if (monitor_thread.joinable()) {
		terminate();
	}
}

void ThreadMonitor::launch() {
	willLaunch();
	setLaunch();
	monitor_thread = std::thread(&ThreadMonitor::thread_func, this, 0);
	didLaunch();
}

void ThreadMonitor::terminate() {
	willTerminate();
	setTerminate();
	monitor_thread.join();
	didTerminate();
}

void SamplingMonitor::setLaunch() {
	status_guard.lock();
	should_continue = true;
	status_guard.unlock();
}

void SamplingMonitor::setTerminate() {
	status_guard.lock();
	should_continue = false;
	status_guard.unlock();
}

void SamplingMonitor::thread_func(int id) {
	willSample();
	int interval = 1e6 / sample_freq;
	while (true) {
		status_guard.lock();
		bool continue_sample = should_continue;
		status_guard.unlock();
		if (continue_sample && !shoudEarlyTerminate()) {
			willSampleOnce();
			sampleOnce();
			didSampleOnce();
		} else {
			break;
		}
		willPause();
		std::this_thread::sleep_for (std::chrono::microseconds(interval));
		didPause();
	}
	didSample();
}

PowerMonitor::PowerMonitor(int freq_in, xocl::device* device_in)
:SamplingMonitor(freq_in) {
	device = device_in;
}

void PowerMonitor::sampleOnce() {
	auto status = readPowerStatus();
	outputPowerStatus(status);
}

void PowerMonitor::didTerminate() {
	power_dump_file.close();
}

void PowerMonitor::willLaunch() {
	std::string dump_filename = "power-trace-"+device->get_unique_name()+".csv";
	std::cout << "open " << dump_filename << std::endl;
	power_dump_file.open(dump_filename, std::ofstream::out | std::ofstream::trunc);
	power_dump_file.close();
	power_dump_file.open(dump_filename, std::ofstream::app);
	power_dump_file << "timestamp" << ",";
	power_dump_file << "mTimeStamp" << ",";
	power_dump_file << "mVInt" << ",";
	power_dump_file << "mCurrent" << ",";
	power_dump_file << "mVAux" << ",";
	power_dump_file << "mVBram" << ",";
	power_dump_file << "m12VPex" << ",";
	power_dump_file << "m12VAux" << ",";
	power_dump_file << "mPexCurr" << ",";
	power_dump_file << "mAuxCurr" << ",";
	power_dump_file << "m3v3Pex" << ",";
	power_dump_file << "m3v3Aux" << ",";
	power_dump_file << "mDDRVppBottom" << ",";
	power_dump_file << "mDDRVppTop" << ",";
	power_dump_file << "mSys5v5" << ",";
	power_dump_file << "m1v2Top" << ",";
	power_dump_file << "m1v8Top" << ",";
	power_dump_file << "m0v85" << ",";
	power_dump_file << "mMgt0v9" << ",";
	power_dump_file << "m12vSW" << ",";
	power_dump_file << "mMgtVtt" << ",";
	power_dump_file << "m1v2Bottom";
	power_dump_file << std::endl;
	power_dump_file.flush();
}

XclPowerInfo PowerMonitor::readPowerStatus() {
	XclPowerInfo powerInfo = device->get_power_info();
	return powerInfo;
}

void PowerMonitor::outputPowerStatus(XclPowerInfo& status) {
	auto timestamp = std::chrono::system_clock::now().time_since_epoch().count();
	power_dump_file << timestamp << ",";
	power_dump_file << status.mTimeStamp << ",";
	power_dump_file << status.mVInt << ",";
	power_dump_file << status.mCurrent << ",";
	power_dump_file << status.mVAux << ",";
	power_dump_file << status.mVBram << ",";
	power_dump_file << status.m12VPex << ",";
	power_dump_file << status.m12VAux << ",";
	power_dump_file << status.mPexCurr << ",";
	power_dump_file << status.mAuxCurr << ",";
	power_dump_file << status.m3v3Pex << ",";
	power_dump_file << status.m3v3Aux << ",";
	power_dump_file << status.mDDRVppBottom << ",";
	power_dump_file << status.mDDRVppTop << ",";
	power_dump_file << status.mSys5v5 << ",";
	power_dump_file << status.m1v2Top << ",";
	power_dump_file << status.m1v8Top << ",";
	power_dump_file << status.m0v85 << ",";
	power_dump_file << status.mMgt0v9 << ",";
	power_dump_file << status.m12vSW << ",";
	power_dump_file << status.mMgtVtt << ",";
	power_dump_file << status.m1v2Bottom;
	power_dump_file << std::endl;
	power_dump_file.flush();
}

PowerProfile::PowerProfile(std::shared_ptr<xocl::platform> platform) {
	for (auto device : platform->get_device_range()) {
      	std::string deviceName = device->get_unique_name();
		BaseMonitor* monitor = new PowerMonitor(100, device);
		powerMonitors.push_back(monitor);
    }
}

void PowerProfile::launch() {
	std::cout << "launching power profile ..." << std::endl;
	for (auto monitor : powerMonitors) {
		monitor->launch();
	}
}

void PowerProfile::terminate() {
	for (auto monitor : powerMonitors) {
		monitor->terminate();
	}
}

}